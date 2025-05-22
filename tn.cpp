/*************************************************************************
    > File Name: tn.cpp
    > Author: frank
    > Mail: 1216451203@qq.com
    > Created Time: 2025年05月21日 星期三 22时22分42秒
 ************************************************************************/

#include "tn.h"
#include <unordered_set>
#include <filesystem>
namespace fs = std::filesystem;

std::unordered_set<int> sentence_splitter = {
    ',',
    '.',
    '!',
    '?',
    ';',
};

// This function mimics Python's len() function.
// It counts each character, treating both letters, Chinese characters and space as 1 unit of length.
// use utf-8 Chinese characters
// no punctuation here!
inline size_t str_len(const std::string& s) {
    int strSize = s.size();
    int i = 0;
    int cnt = 0;
    while (i < strSize) {
        // English letters
        if (s[i] <= 'z' && s[i] >= 'a' || s[i] <= 'Z' && s[i] >= 'A') {
            ++cnt;
            ++i;
        } else {  // Chinese characters
            int len = 1;
            for (int j = 0; j < 6 && (s[i] & (0x80 >> j)); j++) {
                len = j + 1;
            }
            ++cnt;
            i += len;
        }
    }
    return cnt;
}

MeloTn::MeloTn(const std::string& model_dir) {
    std::string punc_dict_dir = model_dir + "/punc.dic";
    _da.open(punc_dict_dir.c_str());
    fs::path dir(model_dir);
    normalizer = std::make_shared<text_normalization::TextNormalizer>(dir);
}

std::vector<std::string> 
MeloTn::split_sentences_zh(const std::string& text, size_t min_len) {
    std::vector<std::string> sentences;
    int n = text.length();
    int MAX_HIT = 1;
    std::string tmp;
    for (int i = 0; i < n;) {
        const char* query = text.data() + i;
        std::vector<Darts::DoubleArray::result_pair_type> results(MAX_HIT);
        size_t num_matches = _da.commonPrefixSearch(query, results.data(), MAX_HIT);
        if (!num_matches) {
            tmp += text[i++];
        } else if ((text[i] == ',' || text[i] == '.') && i > 0 && i < n &&
                   std::isdigit(static_cast<int>(text[i - 1])) && std::isdigit(static_cast<int>(text[i + 1]))) {
            if (text[i] == '.')
                tmp += ".";  // Keep the decimal point here for subsequent text normalization processing.
            i += results.front().length;
        } else if (text[i] == '.' && i + 3 < n && text.substr(i + 1, 3) == "com") {
            tmp += ".";  // Special workaround for .com
            i += results.front().length;
        } else if (sentence_splitter.count(results.front().value)) {  // text splitter
            tmp += static_cast<char>(results.front().value);
            sentences.emplace_back(std::move(tmp));
            tmp.clear();
            i += results.front().length;
        } else if (results.front().value == 3 ||
                   results.front().value == 0) {  // space it is meaningful to english words
            tmp += " ";
            i += results.front().length;
        } else {
            tmp += static_cast<char>(results.front().value);
            i += results.front().length;
        }
    }
    if (tmp.size())
        sentences.emplace_back(std::move(tmp));

    std::vector<std::string> new_sentences;
    size_t count_len = 0;
    std::string new_sent;
    int m = sentences.size();
    for (int i = 0; i < m; ++i) {
        new_sent += sentences[i] + " ";
        count_len += str_len(sentences[i]);
        if (count_len > min_len || i == m - 1) {
            if (new_sent.back() == ' ')
                new_sent.pop_back();
            // new_sent it self is only one piece and it is space, then skip
            if (!std::all_of(new_sent.begin(), new_sent.end(), [&](char& ch) {
                    return ch == ' ';
                }))
                new_sentences.emplace_back(std::move(new_sent));
            new_sent.clear();
            count_len = 0;
        }
    }
    // merge_short_sentences_zh
    // here we fix use the default min_len, so only need to check if the len(new_sentences[-1])<= 2 ;consistent with the
    // Python code
    if (new_sentences.size() >= 2 && str_len(new_sentences.back()) <= 2) {
        new_sentences[new_sentences.size() - 2] += new_sentences.back();
        new_sentences.pop_back();
    }
    return new_sentences;
}


std::vector<std::string> 
MeloTn::split_sentences_into_pieces(const std::string& text, bool quiet) {
    auto pieces = split_sentences_zh(text);
    if (!quiet) {
        std::cout << " > Text split to sentences." << std::endl;
        for (const auto& piece : pieces) {
            std::cout << "   " << piece << std::endl;
        }
        std::cout << " > ===========================" << std::endl;
    }
    return pieces;
}

// Convert uppercase to lowercase
std::string MeloTn::text_normalize(const std::string& text) {
    std::string norm_text = text_normalization::wstring_to_string(
        normalizer->normalize_sentence(text_normalization::string_to_wstring(text)));
    std::for_each(norm_text.begin(), norm_text.end(), [](auto& ch) {
        if (ch <= 'Z' && ch >= 'A')
            ch = ch + 'a' - 'A';
    });
    norm_text = filter_text(norm_text);
    std::cout << "[INFO] normed test is:" << norm_text << std::endl;
    return norm_text;
}

// @brief This functionality cleans up text by retaining only Chinese characters, English letters,
//  and valid punctuation symbols (including space), while removing all other characters.
// UTF-8 is a variable-length encoding that uses 1 to 4 bytes to represent a character.
// It is similar to a Huffman tree in structure. The specific mapping relationship with Unicode is as follows:
// (Adapted from Reference 1)
//
// Unicode Range (Hexadecimal)       UTF-8 Encoding (Binary)
// ----------------------------------------------------------
// 0000 0000 ~ 0000 007F             0xxxxxxx
// 0000 0080 ~ 0000 07FF             110xxxxx 10xxxxxx
// 0000 0800 ~ 0000 FFFF             1110xxxx 10xxxxxx 10xxxxxx
// 0001 0000 ~ 0010 FFFF             11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
//
// UTF-8 is fully compatible with the original ASCII encoding.
// The number of leading 1 bits in the first byte indicates the number of bytes the character occupies.
// Using the table above, Unicode can be converted to UTF-8 encoding by replacing the 'x' placeholders
// with the binary bits of the Unicode value, in high-to-low order, padding with 0s where necessary.
//
// For example, consider the Chinese character "一":
// - Its Unicode code point is 0x4E00, which in binary is: 100 1110 0000 0000 (15 bits).
// - Using the UTF-8 encoding pattern for the range 0000 0800 ~ 0000 FFFF:
//   1110xxxx 10xxxxxx 10xxxxxx
// - Fill in the binary bits of the Unicode code point:
//   - First byte: 1110 + 0100 (first 4 bits of the code point) = 11100100
//   - Second byte: 10 + 111000 (next 6 bits) = 10111000
//   - Third byte: 10 + 000000 (remaining 6 bits) = 10000000
// - Final UTF-8 encoding: 11100100 10111000 10000000 (E4 B8 80 in hexadecimal).
//
// Ref
// https://www.freecodecamp.org/chinese/news/what-is-utf-8-character-encoding/
// https://sf-zhou.github.io/programming/chinese_encoding.html
std::string MeloTn::filter_text(const std::string& input) {
    std::string output;
    size_t i = 0;
    while (i < input.size()) {
        unsigned char first_byte = input[i];
        size_t char_len = 0;
        unsigned int code_point = 0;

        // determine the length of a character (in UTF-8 encoding)
        if ((first_byte & 0x80) == 0x00) {  // 1-byte sequence: 0xxxxxxx
            char_len = 1;
            code_point = first_byte;
        } else if ((first_byte & 0xE0) == 0xC0) {  // 2-byte sequence: 110xxxxx 10xxxxxx
            if (i + 1 >= input.size())
                break;
            char_len = 2;
            code_point = (first_byte & 0x1F) << 6;
            code_point |= (input[i + 1] & 0x3F);
        } else if ((first_byte & 0xF0) == 0xE0) {  // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
            if (i + 2 >= input.size())
                break;
            char_len = 3;
            code_point = (first_byte & 0x0F) << 12;
            code_point |= (input[i + 1] & 0x3F) << 6;
            code_point |= (input[i + 2] & 0x3F);
        } else if ((first_byte & 0xF8) == 0xF0) {  // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            if (i + 3 >= input.size())
                break;
            char_len = 4;
            code_point = (first_byte & 0x07) << 18;
            code_point |= (input[i + 1] & 0x3F) << 12;
            code_point |= (input[i + 2] & 0x3F) << 6;
            code_point |= (input[i + 3] & 0x3F);
        }

        // Determine if the character is a Simplified Chinese or English character
        // or if it is a valid punctuation mark or space
        if (is_chinese_char(code_point) || is_english_char(code_point) || is_valid_punc(code_point) ||
            char_len == 1 && first_byte == ' ') {
            output += input.substr(i, char_len);
        }
        i += char_len;
    }
    return output;
}
