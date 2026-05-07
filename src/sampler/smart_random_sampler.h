/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file smart_random_sampler.h
 * \brief Smart random sampler for faster video random access
 */

#ifndef PELI_SAMPLER_SMART_RANDOM_SAMPLER_H_
#define PELI_SAMPLER_SMART_RANDOM_SAMPLER_H_

#include "sampler_interface.h"

namespace peli {
namespace sampler {

class SmartRandomSampler : public SamplerInterface {
    public:
        SmartRandomSampler(std::vector<int64_t> lens, int interval, int bs_skip);

    private:

};  // class SmartRandomSampler

}  // sampler
}  // peli

#endif  // PELI_SAMPLER_SMART_RANDOM_SAMPLER_H_
