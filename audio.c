#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>

// GUID definitions (needed for compilation)
const CLSID CLSID_MMDeviceEnumerator = {0xBCDE0395, 0xE52F, 0x467C, {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E}};
const IID IID_IMMDeviceEnumerator = {0xA95664D2, 0x9614, 0x4F35, {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6}};
const IID IID_IAudioClient = {0x1CB9AD4C, 0xDBFA, 0x4c32, {0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2}};
const IID IID_IAudioCaptureClient = {0xc8adbd64, 0xe71e, 0x48a0, {0xa4, 0xde, 0x18, 0x5c, 0x39, 0x5c, 0xd3, 0x17}};

const IID IID_IAudioRenderClient = {0xF294ACFC, 0x3146, 0x4483, {0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2}};

int main() {
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pCaptureDevice = NULL;
    IAudioClient *pCaptureClient = NULL;
    WAVEFORMATEX *pCaptureFormat = NULL;
    IAudioCaptureClient *pCaptureClientService = NULL;
    
    IMMDevice *pRenderDevice = NULL;
    IAudioClient *pRenderClient = NULL;
    IAudioRenderClient *pRenderClientService = NULL;
    WAVEFORMATEX *pRenderFormat = NULL;

    int result = 1;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("Failed to initialise COM\n");
        return 1;
    }

    hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        &IID_IMMDeviceEnumerator,
        (void**)&pEnumerator
    );

    if(FAILED(hr)) {
        printf("Failed to create device enumerator\n");
        goto cleanup;
    }

    hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
        pEnumerator,
        eCapture,
        eConsole,
        &pCaptureDevice
    );

    if(FAILED(hr)) {
        printf("Failed to get capture device\n");
        goto cleanup;
        return 1;
    }

    hr = pCaptureDevice->lpVtbl->Activate(
        pCaptureDevice,
        &IID_IAudioClient,
        CLSCTX_ALL,
        NULL,
        (void**)&pCaptureClient
    );

    if(FAILED(hr)) {
        printf("Failed to activate capture client\n");
        goto cleanup;
        return 1;
    }
    
    hr = pCaptureClient->lpVtbl->GetMixFormat(pCaptureClient, &pCaptureFormat);
    if (FAILED(hr)) {
        printf("Failed to get capture format\n");
        goto cleanup;
    }

    printf("Capture Format: \n");
    printf("    Sample Rate: %lu Hz\n", pCaptureFormat->nSamplesPerSec);
    printf("    Channels: %u\n", pCaptureFormat->nChannels);
    printf("    Bits per sample: %u\n", pCaptureFormat->wBitsPerSample);
    printf("    Format tag: %u\n", pCaptureFormat->wFormatTag);

    REFERENCE_TIME hnsRequestedDuration = 100000;
    hr = pCaptureClient->lpVtbl->Initialize(
        pCaptureClient,
        AUDCLNT_SHAREMODE_SHARED,
        0,
        hnsRequestedDuration,
        0,
        pCaptureFormat,
        NULL
    );
    if (FAILED(hr)) {
        printf("Failed to initialise capture client (HRESULT: 0x%lx)\n", hr);
        goto cleanup;
    }

    UINT32 captureBufferFrameCount;
    hr = pCaptureClient->lpVtbl->GetBufferSize(pCaptureClient, &captureBufferFrameCount);
    if (FAILED(hr)) {
        printf("Failed to get capture buffer size\n");
        goto cleanup;
    }

    printf("Capture buffer size: %u frames\n", captureBufferFrameCount);

    hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
        pEnumerator,
        eRender,
        eConsole,
        &pRenderDevice
    );
    if (FAILED(hr)) {
        printf("Failed to get render device\n");
        goto cleanup;
    }

    hr = pRenderDevice->lpVtbl->Activate(
        pRenderDevice,
        &IID_IAudioClient,
        CLSCTX_ALL,
        NULL,
        (void**)&pRenderClient
    );
    if (FAILED(hr)) {
        printf("Failed to activate render client\n");
        goto cleanup;
    }

    hr = pRenderClient->lpVtbl->GetMixFormat(
        pRenderClient,
        &pRenderFormat
    );
    if (FAILED(hr)) {
        printf("Failed to get render format\n");
        goto cleanup;
    }

    printf("\nRender Format: \n");
    printf("    Sample Rate: %lu Hz\n", pRenderFormat->nSamplesPerSec);
    printf("    Channels: %u\n", pRenderFormat->nChannels);
    printf("    Bits per sample: %u\n", pRenderFormat->wBitsPerSample);
    printf("    Format tag: %u\n", pRenderFormat->wFormatTag);

    hr = pRenderClient->lpVtbl->Initialize(
        pRenderClient,
        AUDCLNT_SHAREMODE_SHARED,
        0,
        hnsRequestedDuration,
        0,
        pRenderFormat,
        NULL
    );
    if (FAILED(hr)) {
        printf("Failed to initialise render client (HRESULT: 0x%lx)\n", hr);
        goto cleanup;
    }

    UINT32 renderBufferFrameCount;
    hr = pRenderClient->lpVtbl->GetBufferSize(
        pRenderClient,
        &renderBufferFrameCount
    );
    if (FAILED(hr)) {
        printf("Failed to get render buffer size\n");
        goto cleanup;
    }
    printf("Render buffer size: %u frames\n", renderBufferFrameCount);

    hr = pRenderClient->lpVtbl->GetService(
        pRenderClient,
        &IID_IAudioRenderClient,
        (void**)&pRenderClientService
    );
    if (FAILED(hr)) {
        printf("Failed to get render buffer size\n");
        goto cleanup;
    }

    hr = pCaptureClient->lpVtbl->GetService(
        pCaptureClient,
        &IID_IAudioCaptureClient,
        (void**)&pCaptureClientService
    );
    if (FAILED(hr)) {
        printf("Failed to get capture service\n");
        goto cleanup;
    }

#define BUFFER_SIZE_SECONDS 2
    UINT32 intermediateBufferSize = pCaptureFormat->nSamplesPerSec *
                                    pCaptureFormat->nChannels *
                                    (pCaptureFormat->wBitsPerSample / 8) *
                                    BUFFER_SIZE_SECONDS;
    BYTE *intermediateBuffer = (BYTE*)malloc(intermediateBufferSize);
    if (!intermediateBuffer) {
        printf("Failed to allocate intermediate buffer\n");
        goto cleanup;
    }

    UINT32 writePos = 0;
    UINT32 readPos = 0;
    UINT32 availableData = 0;

    hr = pCaptureClient->lpVtbl->Start(pCaptureClient);
    if (FAILED(hr)) {
        printf("Failed to start capture\n");
        goto cleanup;
    }

    hr = pRenderClient->lpVtbl->Start(pRenderClient);
    if (FAILED(hr)) {
        printf("Failed to start render\n");
        goto cleanup;
    }

    printf("\nLive audio passthrough active! Press Ctrl+C to stop\n");
    printf("Speak into your microphone...\n\n");

    for (int iteration = 0; iteration < 5000; iteration++) {
        Sleep(1);

        UINT32 packetLength = 0;
        hr = pCaptureClientService->lpVtbl->GetNextPacketSize(
            pCaptureClientService,
            &packetLength
        );

        while (packetLength != 0) {
            BYTE *pCaptureData;
            UINT32 numFramesAvailable;
            DWORD flags;

            hr = pCaptureClientService->lpVtbl->GetBuffer(
                pCaptureClientService,
                &pCaptureData,
                &numFramesAvailable,
                &flags,
                NULL,
                NULL
            );

            if (SUCCEEDED(hr)) {
                UINT32 bytesToWrite = numFramesAvailable *
                                        pCaptureFormat->nBlockAlign;

                if (availableData + bytesToWrite <= intermediateBufferSize) {
                    for (UINT32 i = 0; i < bytesToWrite; i++) {
                        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                            intermediateBuffer[writePos] = 0;
                        }
                        else {
                            intermediateBuffer[writePos] = pCaptureData[i];
                        }
                        writePos = (writePos + 1) % intermediateBufferSize;
                    }
                    availableData += bytesToWrite;
                    printf("+");
                }
                else {
                    printf("!");
                }
                fflush(stdout);

                pCaptureClientService->lpVtbl->ReleaseBuffer(
                    pCaptureClientService,
                    numFramesAvailable
                );
            }

            hr = pCaptureClientService->lpVtbl->GetNextPacketSize(
                pCaptureClientService,
                &packetLength
            );
        }

        UINT32 numFramesPadding;
        hr = pRenderClient->lpVtbl->GetCurrentPadding(
            pRenderClient,
            &numFramesPadding
        );
        if (SUCCEEDED(hr)) {
            UINT32 numFramesAvailable = renderBufferFrameCount - numFramesPadding;

            if (numFramesAvailable > 0) {
                BYTE *pRenderData;
                hr = pRenderClientService->lpVtbl->GetBuffer(
                    pRenderClientService,
                    numFramesAvailable,
                    &pRenderData
                );

                if (SUCCEEDED(hr)) {
                    UINT32 bytesToRead = numFramesAvailable * pRenderFormat->nBlockAlign;

                    if (availableData >= bytesToRead) {
                        for (UINT32 i = 0; i < bytesToRead; i++) {
                            pRenderData[i] = intermediateBuffer[readPos];
                            readPos = (readPos + 1) % intermediateBufferSize;
                        }
                        availableData -= bytesToRead;
                        printf("-");
                    }
                    else {
                        memset(pRenderData, 0, bytesToRead);
                        printf("_");
                    }
                    fflush(stdout);

                    pRenderClientService->lpVtbl->ReleaseBuffer(
                        pRenderClientService,
                        numFramesAvailable,
                        0
                    );
                }
            }
        }
        if (iteration % 20 == 0) {
            printf(" [Buffer: %u/%u bytes]\n", availableData, intermediateBufferSize);
        }
    }


    pCaptureClient->lpVtbl->Stop(pCaptureClient);
    pRenderClient->lpVtbl->Stop(pRenderClient);
    printf("\n Stopped audio passthrough\n");

    result = 0;

cleanup:
    if (intermediateBuffer) free(intermediateBuffer);
    if (pRenderClientService) pRenderClientService->lpVtbl->Release(pRenderClientService);
    if (pRenderFormat) CoTaskMemFree(pRenderFormat);
    if (pRenderClient) pRenderClient->lpVtbl->Release(pRenderClient);
    if (pRenderDevice) pRenderDevice->lpVtbl->Release(pRenderDevice);
    if (pCaptureClientService) pCaptureClientService->lpVtbl->Release(pCaptureClientService);
    if (pCaptureFormat) CoTaskMemFree(pCaptureFormat);
    if (pCaptureClient) pCaptureClient->lpVtbl->Release(pCaptureClient);
    if (pCaptureDevice) pCaptureDevice->lpVtbl->Release(pCaptureDevice);
    if (pEnumerator) pEnumerator->lpVtbl->Release(pEnumerator);
    CoUninitialize();
    return result;
}
