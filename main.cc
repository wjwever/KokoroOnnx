#include "kokoro.h"
#include "tn.h"
#include "wave-writer.h"


int main() {
    try {
        std::string model_dir = "./model/";
        std::string jieba_dir = "./dict/";

        std::string kokoro_onnx = model_dir + "/kokoro.onnx";
        std::string tokens = model_dir + "/tokens.txt";
        std::vector<std::string> lexicons = {model_dir + "/lexicon-us-en.txt", model_dir + "/lexicon-zh.txt"};
        std::string voice_bin = model_dir + "/voices.bin";

        Tts tts(kokoro_onnx, tokens, lexicons, voice_bin, jieba_dir);
        //tts.run("来听一听, 这个是什么口音? How are you doing? Are you ok? Thank you! 你觉得中英文说得如何呢?", "zf_001");
        std::string text = "北京时间5月19日多哈世乒赛，王楚钦势如破竹4-0剃光头，零封巴西小将速胜晋级；男单10号种子邱党鏖战七局爆冷被淘汰，从0-3追到3-3，只是最终还是无功而返，下面看看各场对决的简述。王楚钦延续火热的竞技状态，比赛上来连赢七分势不可挡，强力进攻打得对手无可奈何，毫无疑问是做好战术准备，首局几乎没给任何机会11-3速胜。莱昂纳多·饭冢完全被牵制，根本不能发挥自身的优势特点，尝试的变化都无功而返，次局连续遭遇压制心态受到影响，格外的沮丧导致连续发球不太严谨，频频出现非受迫性失误，又是3-11的相同比分落败。第三局莱昂纳多稍有好转迹象，但并无法改变比赛走向，勉强扛住前半段，等到后程又是陷入对手节奏，缺乏绝对得分手段5-11再败。王楚钦发挥几乎无懈可击，看到破绽就果断上手，爆冲拿分格外自信，手握巨大优势没有丝毫松懈，保持专注的态度，11-4轻松终结比赛，总比分4-0完胜晋级男单32强。德国名将邱党3-4不敌贾维斯，比赛宛如坐过山车，0-3落后连扳三局，最后决胜局遗憾落败。邱党世界排名第11，作为本届世乒赛男单的10号种子，个人状态属实不太理想，首轮鏖战七局惊险过关，来到次轮又是极其慢热，前三局比分非常激烈，但关键时刻屡屡掉链子，绝境局面触底反弹，一度看到超级逆转的希望，可惜还是差之毫厘功亏一篑。";
        MeloTn tn(model_dir);


        std::vector<float> data;
        auto pieces = tn.split_sentences_into_pieces(text);

        std::string merged = "";
        for (auto p : pieces) {
            // do a simple merge by . ? !
            merged += p;
            auto b = merged.back();
            if (merged.size() >= 300 or b == '.' or b == '?' or b == '!') {  // 150 means 100 chinese word
                auto nr = tn.text_normalize(merged);
                tts.run(nr, "zf_001", data);
                merged = "";
            } 
        } 
        if (merged.size() > 0) {
            auto nr = tn.text_normalize(merged);
            tts.run(nr, "zf_001", data);
            merged = "";
        }

        sherpa_onnx::WriteWave(std::string("out.wav"), tts._sample_rate, data.data(), data.size());

    }
    catch(std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
