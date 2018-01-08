#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <cctype>
#include <random>
#include <unordered_map>

using StringType = std::string;

struct ApplicationArgs {
	std::optional<std::string> fname;
};

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

std::vector<StringType> tokenize(const StringType &input) {
	std::vector<StringType> tokens;
	std::size_t token_start = 0;
	for (int i = 0; i < input.length(); ++i) {
		bool get_word = false;
		bool get_punctuation = false;
		if (input[i] == '\n') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] >= 0 && input[i] <= 255 && std::isspace(input[i])) {
			get_word = true;
		}
		else if (input[i] == '.') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] == '!') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] == '?') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] == ';') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] == ',') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] == '(') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] == ')') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] == '"') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] == ':') {
			get_punctuation = true;
			get_word = true;
		}
		else if (input[i] == '+') {
			get_punctuation = true;
			get_word = true;
		}

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

	const auto input_contents = read_file(result.fname.value());
	const auto tokens = tokenize(input_contents);
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

	for (int iteration = 0; iteration < 20; ++iteration) {
		std::vector<std::uint64_t> output;
		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_int_distribution<std::uint64_t> dist(0, unique_hashed_tokens.size()-1);
		std::uniform_int_distribution<std::uint64_t> complexity_dist(0, 45);

		uint64_t strain_length = 0;
		for (int i = 0; i < 100; ++i) {
			const uint64_t random_graph_complexity = 99;// complexity_dist(mt);
			if (output.size() > 3 && output[i-3] == output[i-2] && output[i-2] == output[i-1]) {
				break;
			}

			if (random_graph_complexity == 0 || strain_length == 0) {
				uint64_t next_word_id = 0;
				do {
					next_word_id = hash_to_id[unique_hashed_tokens[dist(mt)]];
				} while ( ! std::isalpha(unique_tokens[next_word_id][0]));

				output.push_back(next_word_id);
				strain_length = 1;
			} else if (output.size() == 1 || random_graph_complexity == 1 || strain_length == 1) {
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

			if (unique_tokens[output.back()] == "\n")
				break;
		}

		if (output.size() < 4)
			continue;

		bool capitalize_next_word = true;
		bool first_word = true;
		for (const auto &word : output) {
			if (first_word != true && unique_tokens[word] != "." && unique_tokens[word] != "?" && unique_tokens[word] != "!" && unique_tokens[word] != "," &&
				unique_tokens[word] != ";" && unique_tokens[word] != ":" && unique_tokens[word] != "\"") {
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
			

			if (unique_tokens[word] == "." || unique_tokens[word] == "?" || unique_tokens[word] == "!") {
				capitalize_next_word = true;
			} else {
				capitalize_next_word = false;
			}
		}
	}

	return 0;
}
