#########################################################################
# File Name: build.sh
# Author: frank
# mail: 1216451203@qq.com
# Created Time: 2025年05月12日 星期一 16时01分49秒
#########################################################################
#!/bin/bash
mkdir -p build
cd build
cmake ..
make -j 8

if [ ! -d model ]; then
    wget https://github-1324907443.cos.ap-shanghai.myqcloud.com/kokoro_tts/model.tar.gz -O model.tar.gz 
    tar zxvf model.tar.gz
fi

if [ ! -d dict ];then
    wget https://github.com/csukuangfj/cppjieba/releases/download/sherpa-onnx-2024-04-19/dict.tar.bz2 -O dict.tar.bz2
    tar -xjvf dict.tar.bz2
fi

./bin/infer 
echo "place check wav: build/out.wav"
