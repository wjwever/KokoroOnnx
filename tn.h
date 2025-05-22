/*************************************************************************
    > File Name: tn.h
    > Author: frank
    > Mail: 1216451203@qq.com
    > Created Time: 2025年05月21日 星期三 22时22分21秒
 ************************************************************************/

#include <vector>
#include <memory>
#include <string>
#include <unordered_set>
#include <iostream>
#include "darts.h"
#include "text_normalization.h"

class MeloTn {
public:
    MeloTn(const std::string& model_dir);
    std::vector<std::string> split_sentences_into_pieces(const std::string& text, bool quiet = false); 
    std::shared_ptr<text_normalization::TextNormalizer> normalizer;
    std::string text_normalize(const std::string& text) ;



private:
    const std::unordered_set<char> punctuations =
    {',', '.', '!', '?', ';', '-', '\''};  // After filtering, only these punctuation marks are accepted.

    inline bool is_valid_punc(char x) {
        return punctuations.count(x);
    }
    // Only lowercase letters are accepted in this module!
    inline bool is_english(const std::string& word) {
        for (const auto& ch : word) {
            if (ch < 'a' || ch > 'z')
                return false;
        }
        return true;
    }
    /**
     * The following functions correspond to the Python code:
     * replaced_text = re.sub(r"[^\u4e00-\u9fa5_a-zA-Z\s" + "".join(punctuation) + r"]+", "", replaced_text)
     */
    inline bool is_english_char(unsigned int code_point) {
        return (code_point >= 0x41 && code_point <= 0x5A) || (code_point >= 0x61 && code_point <= 0x7A);
    }
    inline bool is_chinese_char(unsigned int code_point) {
        // Unicode in \u4e00 - \u9fa5）
        return (code_point >= 0x4E00 && code_point <= 0x9FA5);
    }
    Darts::DoubleArray _da;  // punctuation dict use to split sentence
    /*
     * @brief Splits a given text into pieces based on Chinese and English punctuation marks.
     * punctuation marks inlucde {
        "，", "。", "！", "？", "、", "；", "：", "“", "”", "‘", "’", "（", "）", "【", "】", "《", "》", "——", "……", "·",
        ",", ".", "!", "?", ";", ":", "\"", "\"", "'", "'", "(", ")", "[", "]", "<", ">", "-", "...", ".", "\n", "\t", "\r",
        };
       std::unordered_set<std::string> sentence_splitter = {
           "，", "。", "！", "？","；",
           ",", ".", "!", "?", ";",
        };
        std::unordered_set<std::string> spaces = {
            "\n", "\t", "\r",
        };
     * 1. sentence_splitter is split flag; space is space flag; all other punctuaiton marks are filtered
     * 2. In order to keep English word segmentation, whitespace is not included in punctuation marks.
     * 3. If you want to update the puncuation, please use darts.h file (see tests/test_darts.cpp as an example)
    */
    std::vector<std::string> split_sentences_zh(const std::string& text, size_t max_len = 5);
    std::string filter_text(const std::string& input);
};
