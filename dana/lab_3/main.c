#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "comctl32.lib")

#define ID_TIMER_MAIN 1
#define ID_TIMER_POMODORO 2
#define ID_BUTTON_START 1001
#define ID_BUTTON_STOP 1002
#define ID_BUTTON_RESET 1003
#define ID_BUTTON_POMODORO_START 1004
#define ID_BUTTON_POMODORO_STOP 1005
#define ID_EDIT_MINUTES 1006
#define ID_STATIC_TIME 1007
#define ID_STATIC_POMODORO 1008

typedef struct {
    HWND hwnd;
    HWND hStaticTime;
    HWND hStaticPomodoro;
    HWND hEditMinutes;
    
    BOOL isRunning;
    BOOL pomodoroRunning;
    
    int seconds;
    int minutes;
    int hours;
    
    int pomodoroMinutes;
    int pomodoroSeconds;
    int pomodoroPhase;
    
    BOOL notificationActive;
} AppData;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void UpdateDisplay(AppData* data);
void StartStopwatch(AppData* data);
void StopStopwatch(AppData* data);
void ResetStopwatch(AppData* data);
void StartPomodoro(AppData* data);
void StopPomodoro(AppData* data);
void ShowNotification(AppData* data, const char* message);
BOOL CALLBACK MinimizeWindowProc(HWND hwnd, LPARAM lParam);
void MinimizeOtherWindows(HWND hwnd);
void PlayNotificationSound();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "PomodoroTimer";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window class registration error!", "Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        0,
        "PomodoroTimer",
        "Stopwatch + Pomodoro Timer",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 500,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBox(NULL, "Window creation error!", "Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static AppData data = {0};
    
    switch (msg) {
        case WM_CREATE: {
            data.hwnd = hwnd;
            data.isRunning = FALSE;
            data.pomodoroRunning = FALSE;
            data.seconds = 0;
            data.minutes = 0;
            data.hours = 0;
            data.pomodoroMinutes = 25;
            data.pomodoroSeconds = 0;
            data.pomodoroPhase = 0;
            data.notificationActive = FALSE;

            CreateWindow("STATIC", "Stopwatch:", WS_VISIBLE | WS_CHILD,
                        20, 20, 200, 20, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            data.hStaticTime = CreateWindow("STATIC", "00:00:00", WS_VISIBLE | WS_CHILD | SS_CENTER,
                        20, 50, 200, 40, hwnd, (HMENU)ID_STATIC_TIME, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            CreateWindow("BUTTON", "Start", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        20, 100, 80, 30, hwnd, (HMENU)ID_BUTTON_START, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            
            CreateWindow("BUTTON", "Stop", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        110, 100, 80, 30, hwnd, (HMENU)ID_BUTTON_STOP, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            
            CreateWindow("BUTTON", "Reset", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        200, 100, 80, 30, hwnd, (HMENU)ID_BUTTON_RESET, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
                        20, 150, 340, 2, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            CreateWindow("STATIC", "Pomodoro Timer:", WS_VISIBLE | WS_CHILD,
                        20, 170, 200, 20, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            data.hStaticPomodoro = CreateWindow("STATIC", "25:00", WS_VISIBLE | WS_CHILD | SS_CENTER,
                        20, 200, 200, 40, hwnd, (HMENU)ID_STATIC_POMODORO, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            CreateWindow("STATIC", "Work minutes:", WS_VISIBLE | WS_CHILD,
                        20, 250, 100, 20, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            data.hEditMinutes = CreateWindow("EDIT", "25", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                        130, 250, 50, 20, hwnd, (HMENU)ID_EDIT_MINUTES, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            CreateWindow("BUTTON", "Start Pomodoro", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        20, 290, 120, 30, hwnd, (HMENU)ID_BUTTON_POMODORO_START, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            
            CreateWindow("BUTTON", "Stop Pomodoro", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        150, 290, 120, 30, hwnd, (HMENU)ID_BUTTON_POMODORO_STOP, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            CreateWindow("STATIC", "Notification features:\n- Minimize windows\n- Sound alert\n- Background change", 
                        WS_VISIBLE | WS_CHILD,
                        20, 340, 340, 100, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
            if (hFont) {
                SendMessage(data.hStaticTime, WM_SETFONT, (WPARAM)hFont, TRUE);
                SendMessage(data.hStaticPomodoro, WM_SETFONT, (WPARAM)hFont, TRUE);
            }

            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_BUTTON_START:
                    if (!data.isRunning) {
                        StartStopwatch(&data);
                    }
                    break;
                    
                case ID_BUTTON_STOP:
                    if (data.isRunning) {
                        StopStopwatch(&data);
                    }
                    break;
                    
                case ID_BUTTON_RESET:
                    ResetStopwatch(&data);
                    break;
                    
                case ID_BUTTON_POMODORO_START:
                    if (!data.pomodoroRunning) {
                        StartPomodoro(&data);
                    }
                    break;
                    
                case ID_BUTTON_POMODORO_STOP:
                    if (data.pomodoroRunning) {
                        StopPomodoro(&data);
                    }
                    break;
            }
            break;
        }

        case WM_TIMER: {
            if (wParam == ID_TIMER_MAIN && data.isRunning) {
                data.seconds++;
                if (data.seconds >= 60) {
                    data.seconds = 0;
                    data.minutes++;
                    if (data.minutes >= 60) {
                        data.minutes = 0;
                        data.hours++;
                    }
                }
                UpdateDisplay(&data);
            }
            else if (wParam == ID_TIMER_POMODORO && data.pomodoroRunning) {
                if (data.pomodoroSeconds > 0) {
                    data.pomodoroSeconds--;
                } else {
                    if (data.pomodoroMinutes > 0) {
                        data.pomodoroMinutes--;
                        data.pomodoroSeconds = 59;
                    } else {
                        KillTimer(hwnd, ID_TIMER_POMODORO);
                        data.pomodoroRunning = FALSE;
                        
                        if (data.pomodoroPhase == 0) {
                            ShowNotification(&data, "Work time finished! Starting 5 minute break.");
                            data.pomodoroPhase = 1;
                            data.pomodoroMinutes = 5;
                            data.pomodoroSeconds = 0;
                            SetTimer(hwnd, ID_TIMER_POMODORO, 1000, NULL);
                            data.pomodoroRunning = TRUE;
                        } else {
                            ShowNotification(&data, "Break finished! Starting 25 minute work session.");
                            data.pomodoroPhase = 0;
                            data.pomodoroMinutes = 25;
                            data.pomodoroSeconds = 0;
                            SetTimer(hwnd, ID_TIMER_POMODORO, 1000, NULL);
                            data.pomodoroRunning = TRUE;
                        }
                        UpdateDisplay(&data);
                    }
                }
                UpdateDisplay(&data);
            }
            else if (wParam == 3) {
                system("color 07");
                KillTimer(hwnd, 3);
            }
            break;
        }

        case WM_DESTROY: {
            if (data.isRunning) {
                KillTimer(hwnd, ID_TIMER_MAIN);
            }
            if (data.pomodoroRunning) {
                KillTimer(hwnd, ID_TIMER_POMODORO);
            }
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void UpdateDisplay(AppData* data) {
    char buffer[64];
    
    sprintf(buffer, "%02d:%02d:%02d", data->hours, data->minutes, data->seconds);
    SetWindowText(data->hStaticTime, buffer);
    
    if (data->pomodoroRunning) {
        const char* phase = (data->pomodoroPhase == 0) ? "Work: " : "Break: ";
        sprintf(buffer, "%s%02d:%02d", phase, data->pomodoroMinutes, data->pomodoroSeconds);
    } else {
        sprintf(buffer, "%02d:%02d", data->pomodoroMinutes, data->pomodoroSeconds);
    }
    SetWindowText(data->hStaticPomodoro, buffer);
}

void StartStopwatch(AppData* data) {
    data->isRunning = TRUE;
    SetTimer(data->hwnd, ID_TIMER_MAIN, 1000, NULL);
}

void StopStopwatch(AppData* data) {
    data->isRunning = FALSE;
    KillTimer(data->hwnd, ID_TIMER_MAIN);
}

void ResetStopwatch(AppData* data) {
    data->isRunning = FALSE;
    data->seconds = 0;
    data->minutes = 0;
    data->hours = 0;
    KillTimer(data->hwnd, ID_TIMER_MAIN);
    UpdateDisplay(data);
}

void StartPomodoro(AppData* data) {
    char minutesText[10];
    GetWindowText(data->hEditMinutes, minutesText, 10);
    int minutes = atoi(minutesText);
    
    if (minutes > 0 && minutes <= 60) {
        data->pomodoroMinutes = minutes;
        data->pomodoroSeconds = 0;
        data->pomodoroPhase = 0;
        data->pomodoroRunning = TRUE;
        SetTimer(data->hwnd, ID_TIMER_POMODORO, 1000, NULL);
        UpdateDisplay(data);
    } else {
        MessageBox(data->hwnd, "Please enter valid minutes (1-60)", "Error", MB_ICONERROR);
    }
}

void StopPomodoro(AppData* data) {
    data->pomodoroRunning = FALSE;
    KillTimer(data->hwnd, ID_TIMER_POMODORO);
    UpdateDisplay(data);
}

void ShowNotification(AppData* data, const char* message) {
    if (data->notificationActive) return;
    
    data->notificationActive = TRUE;
    
    MinimizeOtherWindows(data->hwnd);
    
    PlayNotificationSound();
    
    if (data->pomodoroPhase == 0) {
        system("color 4F");
    } else {
        system("color 2F");
    }
    
    MessageBox(data->hwnd, message, "Pomodoro Timer", MB_ICONINFORMATION | MB_SYSTEMMODAL);
    
    SetTimer(data->hwnd, 3, 2000, NULL);
    
    data->notificationActive = FALSE;
}

BOOL CALLBACK MinimizeWindowProc(HWND hwnd, LPARAM lParam) {
    HWND keepWindow = (HWND)lParam;
    
    if (hwnd != keepWindow && IsWindowVisible(hwnd) && hwnd != GetDesktopWindow() && hwnd != GetShellWindow()) {
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        if ((style & WS_CAPTION) && (style & WS_SYSMENU)) {
            ShowWindow(hwnd, SW_MINIMIZE);
        }
    }
    return TRUE;
}

void MinimizeOtherWindows(HWND hwnd) {
    EnumWindows(MinimizeWindowProc, (LPARAM)hwnd);
}

void PlayNotificationSound() {
    Beep(1000, 500);
    Sleep(200);
    Beep(1000, 500);
}