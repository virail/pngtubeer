#ifndef UNICODE
#define UNICODE
#endif

// Audio
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WINDOW_WIDTH 1920 * 0.25
#define WINDOW_HEIGHT 1080 * 0.25
#define centerX WINDOW_WIDTH / 2
#define centerY WINDOW_HEIGHT / 2

#define log_info(format, ...) printf("[INFO] - " format "\n", ##__VA_ARGS__)
#define log_error(format, ...) printf("[ERROR] - " format "\n", ##__VA_ARGS__)

static HWND g_hwnd = NULL;
static bool g_running = true;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void RenderFrame(void);

// GUID definitions (needed for compilation)
const CLSID CLSID_MMDeviceEnumerator = {0xBCDE0395, 0xE52F, 0x467C, {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E}};
const IID IID_IMMDeviceEnumerator = {0xA95664D2, 0x9614, 0x4F35, {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6}};
const IID IID_IAudioClient = {0x1CB9AD4C, 0xDBFA, 0x4c32, {0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2}};
const IID IID_IAudioCaptureClient = {0xc8adbd64, 0xe71e, 0x48a0, {0xa4, 0xde, 0x18, 0x5c, 0x39, 0x5c, 0xd3, 0x17}};

const IID IID_IAudioRenderClient = {0xF294ACFC, 0x3146, 0x4483, {0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2}};

static IMMDeviceEnumerator *pEnumerator = NULL;
static IMMDevice *pCaptureDevice = NULL;
static IAudioClient *pCaptureClient = NULL;
static WAVEFORMATEX *pCaptureFormat = NULL;
static IAudioCaptureClient *pCaptureClientService = NULL;

static float g_leftSample = 0.0f;
static float g_rightSample = 0.0f;
static bool g_hasNewAudio = false;

static bool g_keyPress = false;
static bool g_mousePress = false;
UINT_PTR g_timerId = 0;

typedef struct {
    HBITMAP hBitmap;
    int width;
    int height;
} Image;

Image* g_keyboard = NULL;

static HBITMAP hBackBuffer = NULL;
static int bufferWidth = 0, bufferHeight = 0;

void ensure_back_buffer(HDC hdc, int width, int height) {
    if (hBackBuffer == NULL || bufferWidth != width || bufferHeight != height) {
        if (hBackBuffer) DeleteObject(hBackBuffer);

        hBackBuffer = CreateCompatibleBitmap(hdc, width, height);

        bufferWidth = width;
        bufferHeight = height;
    }
}

void CleanupAudio(void) {
    if (pCaptureClientService) pCaptureClientService->lpVtbl->Release(pCaptureClientService);
    if (pCaptureFormat) CoTaskMemFree(pCaptureFormat);
    if (pCaptureClient) pCaptureClient->lpVtbl->Release(pCaptureClient);
    if (pCaptureDevice) pCaptureDevice->lpVtbl->Release(pCaptureDevice);
    if (pEnumerator) pEnumerator->lpVtbl->Release(pEnumerator);
    CoUninitialize();

}

void Cleanup() {
    CleanupAudio();
    if (hBackBuffer) DeleteObject(hBackBuffer);
}

int pngtubeerInit() {
    HRESULT hr;
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
        CleanupAudio();
    }

    hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
        pEnumerator,
        eCapture,
        eConsole,
        &pCaptureDevice
    );

    if(FAILED(hr)) {
        printf("Failed to get capture device\n");
        CleanupAudio();
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
        CleanupAudio();
    }
    
    hr = pCaptureClient->lpVtbl->GetMixFormat(pCaptureClient, &pCaptureFormat);
    if (FAILED(hr)) {
        printf("Failed to get capture format\n");
        CleanupAudio();
    }

    WAVEFORMATEXTENSIBLE *extFormat = (WAVEFORMATEXTENSIBLE*)pCaptureFormat;

    printf("Valid bits per sample: %d\n", extFormat->Samples.wValidBitsPerSample);
    printf("Channel Mask: %08X\n", extFormat->dwChannelMask);
    printf("SubFormat: %08X-%04X-%04X-",
        extFormat->SubFormat.Data1,
        extFormat->SubFormat.Data2,
        extFormat->SubFormat.Data3
    );
    printf("%02X%02X-", extFormat->SubFormat.Data4[0], extFormat->SubFormat.Data4[1]);
    printf("%02X%02X%02X%02X%02X%02X\n",
        extFormat->SubFormat.Data4[2],
        extFormat->SubFormat.Data4[3],
        extFormat->SubFormat.Data4[4],
        extFormat->SubFormat.Data4[5],
        extFormat->SubFormat.Data4[6],
        extFormat->SubFormat.Data4[7]
    );

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
        CleanupAudio();
    }

    UINT32 captureBufferFrameCount;
    hr = pCaptureClient->lpVtbl->GetBufferSize(pCaptureClient, &captureBufferFrameCount);
    if (FAILED(hr)) {
        printf("Failed to get capture buffer size\n");
        CleanupAudio();
    }

    printf("Capture buffer size: %u frames\n", captureBufferFrameCount);

    hr = pCaptureClient->lpVtbl->GetService(
        pCaptureClient,
        &IID_IAudioCaptureClient,
        (void**)&pCaptureClientService
    );
    if (FAILED(hr)) {
        printf("Failed to get capture service\n");
        CleanupAudio();
    }

    hr = pCaptureClient->lpVtbl->Start(pCaptureClient);
    if (FAILED(hr)) {
        printf("Failed to start capture\n");
        CleanupAudio();
    }

    result = 0;

    return result;
}


HRESULT UpdateAudioFrame() {
    if (!pCaptureClient || !pCaptureClientService) {
        return E_POINTER;
    }

    // printf("[INFO] - GOT INTO UPDATE AUDIO FRAME\n");
    HRESULT hr = S_OK;
    UINT32 packetLength = 0;

    hr = pCaptureClientService->lpVtbl->GetNextPacketSize(
        pCaptureClientService,
        &packetLength
    );

    g_hasNewAudio = false;
    g_leftSample = 0.0f;
    g_rightSample = 0.0f;

    while (packetLength > 0) {
        BYTE *pData = NULL;
        UINT32 numFramesAvailable = 0;
        DWORD flags = 0;

        hr = pCaptureClientService->lpVtbl->GetBuffer(
            pCaptureClientService,
            &pData,
            &numFramesAvailable,
            &flags,
            NULL,
            NULL
        );
        if (FAILED(hr)) break;

        if (numFramesAvailable > 0 && !(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
            float *pFloatData = (float*)pData;
            UINT32 lastFrameIndex = numFramesAvailable - 1;

            g_leftSample = pFloatData[lastFrameIndex * 2 + 0];
            g_rightSample = pFloatData[lastFrameIndex * 2 + 1];
            g_hasNewAudio = true;
            // log_info("l:[%f] r:[%f]", g_leftSample, g_rightSample);
        }

        hr = pCaptureClientService->lpVtbl->ReleaseBuffer(
            pCaptureClientService,
            numFramesAvailable
        );
        if (FAILED(hr)) break;

        hr = pCaptureClientService->lpVtbl->GetNextPacketSize(
            pCaptureClientService,
            &packetLength
        );
        if (FAILED(hr)) break;
    }

    return hr;

}

float GetAudioLevel() {
    if (!g_hasNewAudio) return 0.0f;
    return fmaxf(fabsf(g_leftSample), fabsf(g_rightSample));
}

Image* load_image(const char* filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
    if (!data) return NULL;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(NULL);
    void* bits;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, hdcScreen);

    if (!hBitmap) {
        stbi_image_free(data);
        return NULL;
    }

    unsigned char* dest = (unsigned char*)bits;
    for (int i = 0; i < width * height * 4; i += 4) {
        dest[i] = data[i + 2];      // B
        dest[i + 1] = data[i + 1];  // G
        dest[i + 2] = data[i];      // R
        dest[i + 3] = data[i + 3];  // A
    }

    Image* image = (Image*)malloc(sizeof(Image));
    image->hBitmap = hBitmap;
    image->width = width;
    image->height = height;

    stbi_image_free(data);
    return image;

}

void draw_image(HDC hdc, Image* image, int x, int y) {
    if (!image) return;

    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, image->hBitmap);

    BitBlt(hdc, x, y, image->width, image->height, hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOld);
    DeleteDC(hMemDC);
}

void free_image(Image* image) {
    if (image) {
        if (image->hBitmap) {
            DeleteObject(image->hBitmap);
        }
        free(image);
    }
}

void loop(void) {
    MSG msg = {0};

    while (g_running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!g_running) break;

        UpdateAudioFrame();

        // do processing on audio

        RenderFrame();

        Sleep(1);

    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    int result = 1;
    pngtubeerInit();

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PNGTubeerClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Window registration failed", L"Error", MB_OK | MB_ICONERROR);
        CleanupAudio();
    }

    g_hwnd = CreateWindowEx(
        0,
        L"PNGtubeerClass",
        L"PNG tubing app",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    g_timerId = SetTimer(g_hwnd, 1, 16, NULL);

    if (!g_hwnd) {
        MessageBox(NULL, L"Window creation failed", L"Error", MB_OK | MB_ICONERROR);
        CleanupAudio();
        return 1;
    }

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);

    loop();

    Cleanup();
    return 0;

}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg)
    {
        case WM_DESTROY:
            if (g_timerId) {
                KillTimer(hwnd, g_timerId);
            }
            g_running = false;
            PostQuitMessage(0);
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                DestroyWindow(hwnd);
            }
            g_keyPress = true;
            return 0;
        
        case WM_KEYUP:
            g_keyPress = false;
            return 0;

        case WM_TIMER:
            if (wParam == 1) {
                bool anyKeyPressed = false;
                bool anyMousePressed = false;

                if (
                        (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ||
                        (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ||
                        (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
                        ) {
                    anyMousePressed = true;
                }

                for (int vk = 8; vk < 256; vk++) {
                    if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;

                    if (GetAsyncKeyState(vk) & 0x8000) {
                        anyKeyPressed = true;
                        break;
                    }
                }

                if (anyKeyPressed != g_keyPress) {
                    g_keyPress = anyKeyPressed;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                if (anyMousePressed != g_mousePress) {
                    g_mousePress = anyMousePressed;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
        break;
        case WM_ERASEBKGND:
            return 1;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void RenderFrame(void) {
    RECT clientRect;
    GetClientRect(g_hwnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    HDC hdc = GetDC(g_hwnd);
    if (!hdc) return;
    HDC hMemDC = CreateCompatibleDC(hdc);
    ensure_back_buffer(hdc, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBackBuffer);

    SetGraphicsMode(hMemDC, GM_ADVANCED);
    // SetMapMode(hdc, MM_LOENGLISH);

    DPtoLP(hMemDC, (LPPOINT) &clientRect, 2);
    const float middleX = clientRect.right / 2;
    const float middleY = clientRect.bottom / 2;

    // Draw Green Background
    HBRUSH hBrush = CreateSolidBrush(RGB(0,255,0));
    FillRect(hMemDC, &clientRect, hBrush);
    DeleteObject(hBrush);
    
#define mouthWidth 100
#define mouthHeight clientRect.bottom / 10

    RECT bottomMouth = {
        (middleX  - mouthWidth), 
        (middleY),
        (middleX  + mouthWidth),
        (clientRect.bottom)
    };
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH blueBrush = CreateSolidBrush(RGB(100, 100, 255));
    FillRect(hMemDC, &bottomMouth, blueBrush);

    float mouthLevel = GetAudioLevel();
    float audioY = mouthLevel > 0.005f ? (mouthLevel - 0.005f) * 1000.f : 0.0f;

    RECT topMouth = {
        middleX - mouthWidth,
        middleY - audioY - mouthHeight,
        middleX + mouthWidth,
        middleY - audioY
    };
    FillRect(hMemDC, &topMouth, whiteBrush);

    SelectObject(hMemDC, whiteBrush);
#define eyeSize mouthHeight / 4
    // Left eye
    Ellipse(hMemDC,
        middleX - mouthWidth / 2 - eyeSize,
        middleY - mouthHeight / 2 - eyeSize - audioY,
        middleX - mouthWidth / 2 + eyeSize,
        middleY - mouthHeight / 2 + eyeSize - audioY
    );
    // Right eye
    Ellipse(hMemDC,
        middleX + mouthWidth / 2 - eyeSize,
        middleY - mouthHeight / 2 - eyeSize - audioY,
        middleX + mouthWidth / 2 + eyeSize,
        middleY - mouthHeight / 2 + eyeSize - audioY
    );

    float audioLevel = GetAudioLevel();
    if (audioLevel > 0.0f) {
        int barWidth = (int)(audioLevel *500.0f);
        if (barWidth > 200) barWidth = 200;

        HBRUSH barBrush = CreateSolidBrush(RGB(255, 100, 100));
        RECT barRect = {50, 100, 50 + barWidth, 130};
        FillRect(hMemDC, &barRect, barBrush);
        DeleteObject(barBrush);
    }
    // Draw text
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Audio Level: %.4f", audioLevel);
    SetTextColor(hMemDC, RGB(0, 0, 0));
    SetBkMode(hMemDC, TRANSPARENT);
    TextOutA(hMemDC, 10, 20, buffer, (int)strlen(buffer));
    
    // Drawing Left Arm
    float rad = g_keyPress ? -45.0f * (3.14159f / 180.0f) : 45.0f * (3.14159f / 180.0f);
    float c = cos(rad);
    float s = sin(rad);

    float leftX[4] = {-20, -20, 20, 20};
    float leftY[4] = {0, 10, 10, 0};

    POINT points[4] = {
        {leftX[0] * c - leftY[0] * s, leftX[0] * s + leftY[0] * c},
        {leftX[1] * c - leftY[1] * s, leftX[1] * s + leftY[1] * c},
        {leftX[2] * c - leftY[2] * s, leftX[2] * s + leftY[2] * c},
        {leftX[3] * c - leftY[3] * s, leftX[3] * s + leftY[3] * c},
    };

    SaveDC(hMemDC);

    XFORM translate = {1.0f, 0.0f, 0.0f, 1.0f, (FLOAT)middleX - mouthWidth, (FLOAT)middleY};
    ModifyWorldTransform(hMemDC, &translate, MWT_LEFTMULTIPLY);
    
    if (g_keyPress) {
        XFORM translate = {1.0f, 0.0f, 0.0f, 1.0f, -10, 20};
        ModifyWorldTransform(hMemDC, &translate, MWT_LEFTMULTIPLY);
    }

    SelectObject(hMemDC, blueBrush);
    Polygon(hMemDC, points, 4);
    RestoreDC(hMemDC, -1);

    SaveDC(hMemDC);

    rad = g_mousePress ? 45.0f * (3.14159f / 180.0f) : -45.0f * (3.14159f / 180.0f);
    c = cos(rad);
    s = sin(rad);

    float rightX[4] = {-20, -20, 20, 20};
    float rightY[4] = {0, 10, 10, 0};

    POINT rightPoints[4] = {
        {rightX[0] * c - rightY[0] * s, rightX[0] * s + rightY[0] * c},
        {rightX[1] * c - rightY[1] * s, rightX[1] * s + rightY[1] * c},
        {rightX[2] * c - rightY[2] * s, rightX[2] * s + rightY[2] * c},
        {rightX[3] * c - rightY[3] * s, rightX[3] * s + rightY[3] * c},
    };

    XFORM rightTranslate = {1.0f, 0.0f, 0.0f, 1.0f, (FLOAT)middleX + mouthWidth, (FLOAT)middleY};
    ModifyWorldTransform(hMemDC, &rightTranslate, MWT_LEFTMULTIPLY);

    if (g_mousePress) {
        XFORM translate = {1.0f, 0.0f, 0.0f, 1.0f, 10, 20};
        ModifyWorldTransform(hMemDC, &translate, MWT_LEFTMULTIPLY);
    }

    SelectObject(hMemDC, blueBrush);
    Polygon(hMemDC, rightPoints, 4);

    RestoreDC(hMemDC, -1);

    if (!g_keyboard) {
        g_keyboard = load_image("keyboard.png");
    }

    if (g_keyboard) {
        draw_image(hMemDC, g_keyboard, 0, 0);
    }

    DeleteObject(whiteBrush);
    DeleteObject(blueBrush);

    BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOldBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(g_hwnd, hdc);
}
