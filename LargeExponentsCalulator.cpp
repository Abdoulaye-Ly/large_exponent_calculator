#include <windows.h>
#include <commctrl.h>
#include <string>
#include <memory>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cwchar>
#include <future> 
#include <iostream>
#include <vector>
#include <cmath>

//For rounded buttons, new icons, and better UI styling
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

struct DigitNode {
    int digit;
    std::unique_ptr<DigitNode> next;
    DigitNode(int d) : digit(d), next(nullptr) {}
};

class BigNumber {
private:
    void addDigit(int digit) {
        auto newNode = std::make_unique<DigitNode>(digit);
        if (!head) {
            head = std::move(newNode);
        }
        else {
            DigitNode* temp = head.get();
            while (temp->next) {
                temp = temp->next.get();
            }
            temp->next = std::move(newNode);
        }
        length++;
    }


public:
    std::unique_ptr<DigitNode> head;
    size_t length = 0;

    BigNumber() = default;

    BigNumber(int num) {
        if (num == 0) {
            addDigit(0);
            return;
        }

        while (num > 0) {
            addDigit(num % 10);
            num /= 10;
        }
    }

    // Copy constructor
    BigNumber(const BigNumber& other) {
        if (!other.head) return;

        DigitNode* current = other.head.get();
        while (current) {
            addDigit(current->digit);
            current = current->next.get();
        }
    }

    // Copy assignment operator
    BigNumber& operator=(const BigNumber& other) {
        if (this != &other) {
            head.reset();
            length = 0;
            DigitNode* current = other.head.get();
            while (current) {
                addDigit(current->digit);
                current = current->next.get();
            }
        }
        return *this;
    }

    std::string toString() const {
        if (!head) return "0";

        std::string result;
        DigitNode* current = head.get();
        // Collect digits in correct order
        while (current) {
            result = std::to_string(current->digit) + result;
            current = current->next.get();
        }
        return result;
    }


    BigNumber multiply(const BigNumber& other) const {
        BigNumber result;
        int shift = 0;

        DigitNode* currentOther = other.head.get();
        while (currentOther) {
            BigNumber partialSum;
            int carry = 0;

            // Correctly handle leading zeros for alignment
            for (int i = 0; i < shift; i++) {
                partialSum.addDigit(0);
            }

            DigitNode* currentThis = head.get();
            while (currentThis || carry) {
                int product = carry;
                if (currentThis) {
                    product += currentThis->digit * currentOther->digit;
                    currentThis = currentThis->next.get();
                }

                carry = product / 10;
                partialSum.addDigit(product % 10);
            }

            // Add partialSum to result
            result = result.add(partialSum);
            currentOther = currentOther->next.get();
            shift++;
        }

        return result;
    }

    BigNumber add(const BigNumber& other) const {
        BigNumber result;
        int carry = 0;

        DigitNode* p1 = this->head.get();
        DigitNode* p2 = other.head.get();

        while (p1 || p2 || carry) {
            int sum = carry;
            if (p1) {
                sum += p1->digit;
                p1 = p1->next.get();
            }
            if (p2) {
                sum += p2->digit;
                p2 = p2->next.get();
            }

            carry = sum / 10;
            result.addDigit(sum % 10);
        }

        return result;
    }


    BigNumber power(int exponent) const {
        if (exponent == 0) return BigNumber(1);

        BigNumber result(1);
        BigNumber base(*this);

        while (exponent > 0) {
            if (exponent % 2 == 1) {
                result = result.multiply(base);
            }
            base = base.multiply(base);
            exponent /= 2;
        }

        return result;
    }
};

// Global variables
HWND g_hwnd;
HWND g_hEditBase, g_hEditExponent, g_hBtnCalculate;
HWND g_hStaticResult, g_hStaticTime;
HFONT g_hFont;

BigNumber computeExponent(int base, int exponent, std::chrono::duration<double>& computationTime) {
    auto start = std::chrono::high_resolution_clock::now();
    BigNumber result = BigNumber(base).power(exponent);
    computationTime = std::chrono::high_resolution_clock::now() - start;
    return result;
}

void writeToFile(const std::string& content) {
    std::ofstream outFile("exponent_results.txt", std::ios::app);
    if (outFile.is_open()) {
        outFile << "=====================\n";
        outFile << content << "\n";
        outFile << "=====================\n\n";
        outFile.close();
    }
}


void UpdateResultDisplay(const BigNumber& result, double computationTime, int base, int exponent) {
    std::string resultStr = std::to_string(base) + "^" + std::to_string(exponent) + " = " + result.toString();
    SetWindowTextA(g_hStaticResult, resultStr.c_str());

    std::stringstream timeSS;
    timeSS << "Computation time: " << std::fixed << std::setprecision(6) << computationTime << " seconds";
    SetWindowTextA(g_hStaticTime, timeSS.str().c_str());

    // Write to file
    std::stringstream fileContent;
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);

    // Use localtime_s instead of localtime
    struct tm newtime;
    localtime_s(&newtime, &now_time);

    fileContent << std::put_time(&newtime, "%Y-%m-%d %H:%M:%S") << "\n";
    fileContent << base << "^" << exponent << " = " << result.toString() << "\n";
    fileContent << "Computation time: " << computationTime << " seconds";
    writeToFile(fileContent.str());
}

void ValidateInputs() {
    BOOL enableCalculate = TRUE;
    wchar_t baseText[32], exponentText[32];
    GetWindowTextW(g_hEditBase, baseText, 32);
    GetWindowTextW(g_hEditExponent, exponentText, 32);

    if (baseText[0] == L'\0' || exponentText[0] == L'\0') {
        EnableWindow(g_hBtnCalculate, FALSE);
        return;
    }

    int base = _wtoi(baseText);
    int exponent = _wtoi(exponentText);

    if (base < 1 || base > 100 || exponent < 0 || exponent > 100) {
        EnableWindow(g_hBtnCalculate, FALSE);
    }
    else {
        EnableWindow(g_hBtnCalculate, TRUE);
    }
}

void OnCalculate() {
    wchar_t baseText[32], exponentText[32];
    GetWindowTextW(g_hEditBase, baseText, 32);
    GetWindowTextW(g_hEditExponent, exponentText, 32);

    try {
        int base = _wtoi(baseText);
        int exponent = _wtoi(exponentText);

        // Run exponentiation in a background thread to prevent UI from freezing while computing large powers
        std::future<BigNumber> futureResult = std::async(std::launch::async, [&]() {
            std::chrono::duration<double> computationTime;
            return computeExponent(base, exponent, computationTime);
            });

        BigNumber result = futureResult.get(); // Get the result when ready
        UpdateResultDisplay(result, 0, base, exponent);
    }
    catch (const std::exception& e) {
        MessageBoxA(g_hwnd, e.what(), "Error", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Create font
        g_hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        // Create controls
        CreateWindowW(L"STATIC", L"Large Exponent Calculator",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            10, 10, 560, 30, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L"Base (m):",
            WS_VISIBLE | WS_CHILD,
            50, 60, 100, 25, hwnd, NULL, NULL, NULL);
        g_hEditBase = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            150, 60, 100, 25, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L"Exponent (n):",
            WS_VISIBLE | WS_CHILD,
            50, 100, 100, 25, hwnd, NULL, NULL, NULL);
        g_hEditExponent = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            150, 100, 100, 25, hwnd, NULL, NULL, NULL);

        g_hBtnCalculate = CreateWindowW(L"BUTTON", L"Calculate",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            150, 140, 100, 30, hwnd, (HMENU)1, NULL, NULL);
        EnableWindow(g_hBtnCalculate, FALSE);

        g_hStaticResult = CreateWindowW(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            50, 190, 500, 50, hwnd, NULL, NULL, NULL);
        g_hStaticTime = CreateWindowW(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            50, 240, 500, 25, hwnd, NULL, NULL, NULL);

        // Set fonts for all controls
        HWND hChild = GetWindow(hwnd, GW_CHILD);
        while (hChild) {
            SendMessage(hChild, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) { // Calculate button
            OnCalculate();
        }
        else if (HIWORD(wParam) == EN_CHANGE) { // Edit control text changed
            ValidateInputs();
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    case WM_DESTROY: {
        DeleteObject(g_hFont);
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}


// Tests
#ifdef RUN_TESTS
#include <iostream>
#include <string>
#include <cassert>

class ExponentCalculatorTests {
public:
    static void RunAllTests() {
        TestBasicOperations();
        TestEdgeCases();
        TestPerformance();
        TestAdditionalCases();  // New test cases
        BenchmarkPerformance(); // Performance tests
    }

private:
    // Helper function now properly declared as static member
    static void VerifyResult(int base, int exponent, const std::string& expected) {
        std::chrono::duration<double> computationTime;
        BigNumber result = computeExponent(base, exponent, computationTime);
        if (result.toString() != expected) {
            std::cerr << "FAILED: " << base << "^" << exponent
                << " expected " << expected << " but got " << result.toString() << std::endl;
            assert(false);
        }
    }

    static void TestBasicOperations() {
        VerifyResult(2, 2, "4");
        VerifyResult(3, 3, "27");
        VerifyResult(5, 5, "3125");
    }

    static void TestEdgeCases() {
        VerifyResult(0, 0, "1");
        VerifyResult(1, 100, "1");
        VerifyResult(10, 0, "1");
    }

    static void TestPerformance() {
        std::chrono::duration<double> computationTime;
        BigNumber result = computeExponent(2, 20, computationTime);
        std::cout << "2^20 (" << result.toString().length()
            << " digits) calculated in " << computationTime.count() << "s" << std::endl;
    }

    static void TestAdditionalCases() {
        VerifyResult(2, 10, "1024");
        VerifyResult(3, 10, "59049");
        VerifyResult(12, 3, "1728");
        VerifyResult(15, 3, "3375");
        VerifyResult(10, 5, "100000");
    }

    static void BenchmarkPerformance() {
        std::cout << "\nPerformance Benchmark:\n";
        TimeCalculation(2, 20);
        TimeCalculation(3, 15);
        TimeCalculation(5, 10);
        TimeCalculation(10, 6);
    }

    static void TimeCalculation(int base, int exponent) {
        auto start = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> computationTime;
        BigNumber result = computeExponent(base, exponent, computationTime);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << base << "^" << exponent << " (" << result.toString().length()
            << " digits): " << elapsed.count() << "s\n";
    }
};

int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    ExponentCalculatorTests::RunAllTests();
    std::cout << "All tests completed!\nPress Enter to exit...";
    std::cin.ignore();
    return 0;
}

#else



int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{

   
    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    // Register window class
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PowerOfListsClass";
    RegisterClassW(&wc);

    // Create window
    g_hwnd = CreateWindowW(L"PowerOfListsClass", L"Power of Lists",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        NULL, NULL, hInstance, NULL);

    if (!g_hwnd) return 0;

    

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
#endif
