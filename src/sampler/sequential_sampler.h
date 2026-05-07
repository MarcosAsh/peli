/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file sequential_sampler.h
 * \brief Sequential sampler, with fixed reading order
 */

#ifndef PELI_SAMPLER_SEQUENTIAL_SAMPLER_H_
#define PELI_SAMPLER_SEQUENTIAL_SAMPLER_H_

#include "sampler_interface.h"

namespace peli {
namespace sampler {

class SequentialSampler : public SamplerInterface {
    public:
        SequentialSampler(std::vector<int64_t> lens, std::vector<int64_t> range, int bs, int interval, int skip);
        ~SequentialSampler() = default;
        void Reset();
        bool HasNext() const;
        const Samples& Next();
        size_t Size() const;

    private:
        size_t bs_;
        Samples samples_;
        size_t curr_;
        std::vector<Samples> visit_order_;

};  // class SequentialSampler

}  // sampler
}  // peli

#endif  // PELI_SAMPLER_SEQUENTIAL_SAMPLER_H_
