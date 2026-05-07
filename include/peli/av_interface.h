//
// Created by Yin, Weisu on 1/7/21.
//

#ifndef PELI_AV_INTERFACE_H
#define PELI_AV_INTERFACE_H

#include "runtime/ndarray.h"

namespace peli {

    using NDArray = runtime::NDArray;

    struct AVContainer {
        NDArray audio;
        NDArray video;
    };

    class AVReaderInterface {

    public:
        virtual AVContainer GetBatch(std::vector<int64_t> indices, AVContainer buf) = 0;
        virtual ~AVReaderInterface() = default;

    };
}

#endif //PELI_AV_INTERFACE_H
