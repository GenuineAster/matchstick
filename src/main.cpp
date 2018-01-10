#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <cctype>
#include <random>
#include <unordered_map>
#include "../ext/json/include/tao/json.hpp"
#include <regex>

using StringType = std::string;

struct ApplicationArgs {
	std::optional<std::string> fname;
};

struct Separator {
	std::string token;
};

struct Punctuation {
	std::string token;
	bool capitalize_next = false;
	bool terminate_chain = false;
	bool space_before = false;
};

struct SpecialToken {
	std::string token;
};

bool operator==(const SpecialToken &a, const std::string &b) {
	return a.token == b;
}

bool operator==(const std::string &a, const SpecialToken &b) {
	return b == a;
}

struct MarkovConfig {
	std::optional<std::string> source;
	std::vector<Punctuation> punctuation;
	std::vector<Separator> separators;
	std::vector<SpecialToken> special_tokens;
	std::regex ignore_regex;
	std::uint64_t minimum_chain_tokens;
};

MarkovConfig parse_config(const tao::json::value &value) {
	MarkovConfig config;
	if (const auto source_json = value.find("source"); source_json && source_json->is_string()) {
		config.source = source_json->get_string();
	}

	if (const auto source_json = value.find("minimum-chain-tokens"); source_json && source_json->is_integer()) {
		config.minimum_chain_tokens = source_json->get_unsigned();
	}

	if (const auto ignore_json = value.find("ignore-token"); ignore_json && ignore_json->is_string()) {
		config.ignore_regex = std::regex(ignore_json->get_string(), std::regex_constants::syntax_option_type::icase | std::regex_constants::syntax_option_type::extended | std::regex_constants::syntax_option_type::optimize );
	}

	if (const auto punctuation_array_json = value.find("punctuation"); punctuation_array_json && punctuation_array_json->is_array()) {
		for (const auto &punctuation_json : punctuation_array_json->get_array()) {
			Punctuation p;
			if (const auto &p_value = punctuation_json.find("value"); p_value && p_value->is_string()) {
				p.token = p_value->get_string();
			}
			if (const auto &p_value = punctuation_json.find("capitalize-next"); p_value && p_value->is_boolean()) {
				p.capitalize_next = p_value->get_boolean();
			}
			if (const auto &p_value = punctuation_json.find("terminate-chain"); p_value && p_value->is_boolean()) {
				p.terminate_chain = p_value->get_boolean();
			}
			if (const auto &p_value = punctuation_json.find("space-before"); p_value && p_value->is_boolean()) {
				p.space_before = p_value->get_boolean();
			}
			config.punctuation.push_back(p);
		}
	}

	if (const auto separators_array_json = value.find("separators"); separators_array_json && separators_array_json->is_array()) {
		for (const auto &separators_json : separators_array_json->get_array()) {
			Separator s;
			if (const auto &s_value = separators_json.find("value"); s_value && s_value->is_string()) {
				s.token = s_value->get_string();
			}
			config.separators.push_back(s);
		}
	}

	if (const auto special_tokens_array_json = value.find("special-tokens"); special_tokens_array_json && special_tokens_array_json->is_array()) {
		for (const auto &special_tokens_json : special_tokens_array_json->get_array()) {
			SpecialToken s;
			if (const auto &s_value = special_tokens_json.find("value"); s_value && s_value->is_string()) {
				s.token = s_value->get_string();
			}
			config.special_tokens.push_back(s);
		}
	}

	return config;
}

ApplicationArgs parse_args(int argc, char **argv) {
	ApplicationArgs result;
	for (int i = 1; i < argc; ++i) {
		std::string curr_arg = argv[i];
		if (curr_arg == "-f") {
			if (i+1 < argc) {
				result.fname = argv[i+1];
			} else {
				std::cerr << "Invalid argument to -f" << std::endl;
			}
		}
	}
	return result;
}

StringType read_file(const std::string &filename) {
	if (std::ifstream f(filename, std::ios::binary); f.is_open()) {
		return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
	}
	return {};
};

std::vector<StringType> tokenize(const MarkovConfig &config, const StringType &input) {
	std::vector<StringType> tokens;
	std::size_t token_start = 0;
	for (int i = 0; i < input.length(); ++i) {
		// process special tokens and ignored tokens first
		{
			int whitespace_index = i;
			while ((input[whitespace_index] < -1 || input[whitespace_index] > 255 || ! std::isspace(input[whitespace_index])) && whitespace_index < input.size())
				++whitespace_index;
			const std::string tok = input.substr(i, whitespace_index-i);
			if (std::regex_match(tok, config.ignore_regex)) {
				token_start = whitespace_index+1;
				i = whitespace_index+1;
				continue;
			} else if (std::find(config.special_tokens.begin(), config.special_tokens.end(), tok) != config.special_tokens.end()) {
				// special tokens are stored as-is and are not modified or split by the tokenize engine
				token_start = whitespace_index+1;
				i = whitespace_index+1;
				tokens.push_back(tok);
				continue;
			}
		}

		bool get_word = false;
		bool get_punctuation = false;
		for (const auto &p : config.punctuation) {
			if (input.length()-i > p.token.length()) {
				if (input.substr(i, p.token.length()) == p.token) {
					get_punctuation = true;
					get_word = true;
				}
			}
		}

		for (const auto &s : config.separators) {
			if (input.length()-i > s.token.length()) {
				if (input.substr(i, s.token.length()) == s.token) {
					get_word = true;
				}
			}
		}

		if (i == input.size()-1)
			get_word = true;

		if (get_word) {
			if (i-token_start >= 1) {
				tokens.push_back(input.substr(token_start, i-token_start));
				for (auto &c : tokens.back()) {
					c = std::tolower(c);
				}
			}
			token_start = i+1;
		}
		if (get_punctuation) {
			tokens.push_back(StringType(input.substr(i, 1)));
		}
	}
	return tokens;
}

std::vector<std::uint64_t> hash_tokens(const std::vector<StringType> &tokens) {
	std::vector<std::uint64_t> result;
	result.reserve(tokens.size());
	for (int i = 0; i < tokens.size(); ++i) {
		result.push_back(std::hash<StringType>()(tokens[i]));
	}
	return result;
}

template<typename T>
std::vector<T> copy_unique(const std::vector<T> &in) {
	std::vector<T> out = in;
	std::sort(out.begin(), out.end());
	auto end = std::unique(out.begin(), out.end());
	out.erase(end, out.end());
	return out;
}

int main(int argc, char **argv) {
	const auto result = parse_args(argc, argv);
	if ( ! result.fname.has_value()) {
		std::cerr << "No filename specified" << std::endl;
		return -1;
	}

	const auto json_config = tao::json::parse_file(result.fname.value());

	const auto markov_json = json_config.find("markov");
	if ( ! markov_json->is_object()) {
		std::cerr << "Couldn't find markov config" << std::endl;
		return -1;
	}
	
	const MarkovConfig config = parse_config(*markov_json);

	const auto input_contents = read_file(config.source.value());
	const auto tokens = tokenize(config, input_contents);
	const auto unique_tokens = copy_unique(tokens);
	const auto hashed_tokens = hash_tokens(tokens);
	const auto unique_hashed_tokens = hash_tokens(unique_tokens);

	std::unordered_map<std::uint64_t, std::uint64_t> hash_to_id;
	for (int i = 0; i < unique_hashed_tokens.size(); ++i) {
		hash_to_id[unique_hashed_tokens[i]] = i;
	}

	std::vector<std::uint64_t> id_translation;
	id_translation.reserve(hashed_tokens.size());
	for (const auto &hash : hashed_tokens) {
		id_translation.push_back(hash_to_id[hash]);
	}

	std::unordered_map<std::uint64_t, std::unordered_map<std::uint64_t, std::vector<std::uint64_t>>> graph;

	for (int i = 1; i < id_translation.size()-1; ++i) {
		graph[id_translation[i-1]][id_translation[i]].push_back(id_translation[i+1]);
	}

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<std::uint64_t> dist(0, unique_hashed_tokens.size()-1);

	for (int iteration = 0; iteration < 20; ++iteration) {
		std::vector<std::uint64_t> output;

		uint64_t strain_length = 0;
		for (int i = 0; i < 100; ++i) {
			if (output.size() > 3 && output[i-3] == output[i-2] && output[i-2] == output[i-1]) {
				break;
			}

			if (strain_length == 0) {
				uint64_t next_word_id = 0;
				do {
					next_word_id = hash_to_id[unique_hashed_tokens[dist(mt)]];
				} while (unique_tokens[next_word_id][0] >= -1 && unique_tokens[next_word_id][0] <= 255 && std::isalpha(unique_tokens[next_word_id][0]) == false);

				output.push_back(next_word_id);
				strain_length = 1;
			} else if (output.size() == 1 || strain_length == 1) {
				const auto &next_vec = graph[output[i-1]];
				if (next_vec.empty())
					break;

				std::uniform_int_distribution<std::uint64_t> next_dist(0, next_vec.size()-1);
				auto it = next_vec.begin();
				const auto dist = next_dist(mt);
				std::advance(it, dist);
				output.push_back(it->first);
				strain_length = 2;
			} else {
				const auto &next_vec = graph[output[i-2]][output[i-1]];
				if (next_vec.empty())
					break;

				std::uniform_int_distribution<std::uint64_t> next_dist(0, next_vec.size()-1);
				auto it = next_vec.begin();
				const auto dist = next_dist(mt);
				std::advance(it, dist);
				output.push_back(*it);
				strain_length = 3;
			}

			if (std::find_if(config.punctuation.begin(), config.punctuation.end(), [tok=unique_tokens[output.back()]] (const auto &a) {
				return a.token == tok && a.terminate_chain;
			}) != config.punctuation.end())
				break;
		}

		if (output.size() < config.minimum_chain_tokens)
			continue;

		bool capitalize_next_word = true;
		bool first_word = true;
		for (const auto &word : output) {
			bool is_punctuation = false;
			bool print_space = false;
			for (const auto &punctuation : config.punctuation) {
				if (punctuation.token == unique_tokens[word]) {
					is_punctuation = true;
					if (punctuation.space_before == true) {
						print_space = true;
						break;
					}
				}
			}

			if ( ! is_punctuation) {
				print_space = true;
			}

			if ( ! first_word && print_space) {
				std::cout << " ";
			}

			if (first_word)
				first_word = false;

			if (capitalize_next_word) {
				std::cout << (char)std::toupper(unique_tokens[word][0]) << unique_tokens[word].substr(1);
			}
			else {
				std::cout << unique_tokens[word];
			}
			

			capitalize_next_word = false;
			for (const auto &punctuation : config.punctuation) {
				if (punctuation.capitalize_next == true && punctuation.token == unique_tokens[word]) {
					capitalize_next_word = true;
					break;
				}
			}
		}
		std::cout << std::endl;
	}

	return 0;
}
