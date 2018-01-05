#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <cctype>
#include <random>
#include <unordered_map>

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

std::string read_file(const std::string &filename) {
	if (std::ifstream f(filename); f.is_open()) {
		return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
	}
	return "";
};



std::vector<std::string> tokenize(const std::string &input) {
	std::vector<std::string> tokens;
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
			tokens.push_back(std::string(input.substr(i, 1)));
		}
	}
	return tokens;
}

std::vector<std::uint64_t> hash_tokens(const std::vector<std::string> &tokens) {
	std::vector<std::uint64_t> result;
	result.reserve(tokens.size());
	for (int i = 0; i < tokens.size(); ++i) {
		result.push_back(std::hash<std::string>()(tokens[i]));
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

	std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> graph;

	for (int i = 0; i < id_translation.size()-1; ++i) {
		graph[id_translation[i]].push_back(id_translation[i+1]);
	}

	std::vector<std::uint64_t> output;
	std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<std::uint64_t> dist(0, unique_hashed_tokens.size()-1);
	uint64_t first_word = 0;
	do {
		first_word = hash_to_id[unique_hashed_tokens[dist(mt)]];
	} while ( ! std::isalpha(unique_tokens[first_word][0]));

	output.push_back(first_word);

	for (int i = 1; i < 100; ++i) {
		const auto &next_vec = graph[output.back()];
		if (next_vec.empty())
			break;

		std::uniform_int_distribution<std::uint64_t> next_dist(0, next_vec.size()-1);
		output.push_back(next_vec[next_dist(mt)]);
	}

	//bool insert_space = false;
	bool capitalize_next_word = true;
	for (const auto &word : output) {
		if (unique_tokens[word] == "." || unique_tokens[word] == "?" || unique_tokens[word] == "!" || unique_tokens[word] == ","
			|| unique_tokens[word] == ";" || unique_tokens[word] == ":" || unique_tokens[word] == "\"") {
			//insert_space = true;
		} else {
			std::cout << " ";
		}

		if (capitalize_next_word) {
			std::cout << (char)std::toupper(unique_tokens[word][0]) << unique_tokens[word].substr(1);
		}
		else {
			std::cout << unique_tokens[word];
		}

		if (unique_tokens[word] == "." || unique_tokens[word] == "?" || unique_tokens[word] == "!") {
			//insert_space = true;
			capitalize_next_word = true;
		} else {
			capitalize_next_word = false;
		}
	}

	return 0;
}
