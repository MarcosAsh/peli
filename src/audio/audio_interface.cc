//
// Created by Yin, Weisu on 1/15/21.
//

#include "audio_reader.h"


#include <peli/audio_interface.h>
#include <peli/runtime/registry.h>


namespace peli {

    AudioReaderPtr GetAudioReader(std::string fn, int sampleRate, DLContext ctx, int io_type, bool mono) {
        std::shared_ptr<AudioReaderInterface> ptr;
        ptr = std::make_shared<AudioReader>(fn, sampleRate, ctx, io_type, mono);
        return ptr;
    }


    namespace runtime {
        PELI_REGISTER_GLOBAL("audio_reader._CAPI_AudioReaderGetAudioReader")
        .set_body([](PELIArgs args, PELIRetValue* rv) {
            std::string fn = args[0];
            int device_type = args[1];
            int device_id = args[2];
            int sampleRate = args[3];
            int io_type = args[4];
            bool mono = int(args[5]) == 1 ? true : false;

            // TODO: add io type
            DLContext ctx;
            ctx.device_type = static_cast<DLDeviceType>(device_type);
            ctx.device_id = device_id;
            auto reader = new AudioReader(fn, sampleRate, ctx, io_type, mono);
            if (reader->GetNumSamplesPerChannel() <= 0) {
                *rv = nullptr;
                return;
            }
            AudioReaderInterfaceHandle handle = static_cast<AudioReaderInterfaceHandle>(reader);
            *rv = handle;
        });

        PELI_REGISTER_GLOBAL("audio_reader._CAPI_AudioReaderGetNDArray")
        .set_body([](PELIArgs args, PELIRetValue* rv) {
            AudioReaderInterfaceHandle handle = args[0];
            NDArray array = static_cast<AudioReaderInterface*>(handle)->GetNDArray();
            *rv = array;
        });

        PELI_REGISTER_GLOBAL("audio_reader._CAPI_AudioReaderGetNumPaddingSamples")
        .set_body([](PELIArgs args, PELIRetValue* rv) {
            AudioReaderInterfaceHandle handle = args[0];
            int numPaddingSamples = static_cast<AudioReaderInterface*>(handle)->GetNumPaddingSamples();
            *rv = numPaddingSamples;
        });

        PELI_REGISTER_GLOBAL("audio_reader._CAPI_AudioReaderGetDuration")
        .set_body([](PELIArgs args, PELIRetValue* rv) {
            AudioReaderInterfaceHandle handle = args[0];
            double duration = static_cast<AudioReaderInterface*>(handle)->GetDuration();
            *rv = duration;
        });

        PELI_REGISTER_GLOBAL("audio_reader._CAPI_AudioReaderGetNumSamplesPerChannel")
        .set_body([](PELIArgs args, PELIRetValue* rv) {
            AudioReaderInterfaceHandle handle = args[0];
            int64_t numSamplesPerChannel = static_cast<AudioReaderInterface*>(handle)->GetNumSamplesPerChannel();
            *rv = numSamplesPerChannel;
        });

        PELI_REGISTER_GLOBAL("audio_reader._CAPI_AudioReaderGetNumChannels")
        .set_body([](PELIArgs args, PELIRetValue* rv) {
            AudioReaderInterfaceHandle handle = args[0];
            int numChannels = static_cast<AudioReaderInterface*>(handle)->GetNumChannels();
            *rv = numChannels;
        });

        PELI_REGISTER_GLOBAL("audio_reader._CAPI_AudioReaderGetInfo")
        .set_body([](PELIArgs args, PELIRetValue* rv) {
            AudioReaderInterfaceHandle handle = args[0];
            static_cast<AudioReaderInterface*>(handle)->GetInfo();
        });

        PELI_REGISTER_GLOBAL("audio_reader._CAPI_AudioReaderFree")
        .set_body([](PELIArgs args, PELIRetValue* rv) {
            AudioReaderInterfaceHandle handle = args[0];
            auto p = static_cast<AudioReaderInterface*>(handle);
            if (p) delete p;
        });
    }
}