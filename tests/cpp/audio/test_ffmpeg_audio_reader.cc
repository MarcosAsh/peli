//
// Created by Yin, Weisu on 1/15/21.
//

#include <peli/audio_interface.h>
#include <peli/base.h>


int main() {
    auto audioReader = peli::GetAudioReader("/Users/weisy/Developer/yinweisu/peli/examples/example.mp3", -1, peli::kCPU, 0, 0);
    return 0;
}