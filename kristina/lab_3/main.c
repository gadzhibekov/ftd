#include <windows.h>
#include <stdio.h>
#include <time.h>

#define ID_TIMER 1
#define WM_PET_NOTIFICATION (WM_USER + 100)
#define ID_BUTTON_FEED 2001

typedef struct
{
    int     hunger;
    int     happiness;
    int     energy;
    int     health;
    wchar_t name[50];
    int     is_sleeping;
} Pet;

Pet             myPet;
HWND            hMainWnd;
HWND            hFeedButton;
NOTIFYICONDATAW nid;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void InitializePet();
void UpdatePet();
void ShowPetStatus();
void ShowNotification(const wchar_t* title, const wchar_t* message);
void DrawPet(HDC hdc, RECT rect);
void FeedPet();
void CheckHappinessState(int oldHappiness, int newHappiness);

// Using WinMain instead of wWinMain for MinGW compatibility
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    static wchar_t szAppName[] = L"PetSimulator";
    HWND hwnd;
    MSG msg;
    WNDCLASSEXW wndclass;

    InitializePet();

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);  // Type cast
    wndclass.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);      // Type cast
    wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = szAppName;
    wndclass.hIconSm        = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);  // Type cast

    if (!RegisterClassExW(&wndclass))
    {
        MessageBoxW(NULL, L"Window class registration error!", L"Error", MB_ICONERROR);
        return 0;
    }

    hwnd = CreateWindowW(szAppName,
                        L"Pet Simulator",
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        400,
                        500,
                        NULL,
                        NULL,
                        hInstance,
                        NULL);

    if (!hwnd)
    {
        MessageBoxW(NULL, L"Window creation error!", L"Error", MB_ICONERROR);
        return 0;
    }

    hMainWnd = hwnd;

    nid.cbSize              = sizeof(NOTIFYICONDATAW);
    nid.hWnd                = hwnd;
    nid.uID                 = 100;
    nid.uFlags              = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    nid.uCallbackMessage    = WM_PET_NOTIFICATION;
    nid.hIcon               = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);  // Type cast
    wcscpy(nid.szTip, L"Pet Simulator");
    Shell_NotifyIconW(NIM_ADD, &nid);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    SetTimer(hwnd, ID_TIMER, 2000, NULL);

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Shell_NotifyIconW(NIM_DELETE, &nid);

    return (int)msg.wParam;
}

void InitializePet()
{
    wcscpy(myPet.name, L"Fluffy");
    myPet.hunger = 50;
    myPet.happiness = 70;
    myPet.energy = 80;
    myPet.health = 90;
    myPet.is_sleeping = 0;
}

void CheckHappinessState(int oldHappiness, int newHappiness)
{
    // Проверяем переход через границу 90
    if (oldHappiness < 90 && newHappiness >= 90)
    {
        ShowNotification(L"Your pet is happy", L"Your pet became happy and is enjoying life!");
    }
    else if (oldHappiness >= 90 && newHappiness < 90)
    {
        ShowNotification(L"Your pet is sad", L"Your pet became sad and needs attention!");
    }
}

void UpdatePet()
{
    int oldHappiness = myPet.happiness;

    if (!myPet.is_sleeping)
    {
        myPet.hunger    = min(100, myPet.hunger + 3);
        myPet.happiness -= 3;
        myPet.energy    -= 4;
    }
    else
    {
        myPet.energy += 8;
    }

    myPet.health = max(0, myPet.health - 2);

    myPet.hunger    = max(0, min(100, myPet.hunger));
    myPet.happiness = max(0, min(100, myPet.happiness));
    myPet.energy    = max(0, min(100, myPet.energy));
    myPet.health    = max(0, min(100, myPet.health));

    if (myPet.hunger > 80 || myPet.happiness < 20)
        myPet.health = max(0, myPet.health - 2);

    // Проверяем изменение состояния счастья
    CheckHappinessState(oldHappiness, myPet.happiness);

    if (myPet.energy < 30 && !myPet.is_sleeping)
    {
        myPet.is_sleeping = 1;
        ShowNotification(L"Pet is sleeping", L"Your pet got tired and fell asleep to regain energy!");
    }
    else if (myPet.energy > 70 && myPet.is_sleeping)
    {
        myPet.is_sleeping = 0;
        ShowNotification(L"Pet woke up", L"Your pet slept well and is full of energy and ready to play!");
    }

    if (myPet.hunger > 80)
        ShowNotification(L"Pet is hungry", L"Your pet is very hungry and needs to be fed immediately!");
    else if (myPet.health < 30)
        ShowNotification(L"Pet is sick", L"Your pet is not feeling well and needs medical attention!");
}

void ShowNotification(const wchar_t* title, const wchar_t* message)
{
    nid.uFlags = NIF_INFO;
    wcscpy(nid.szInfoTitle, title);
    wcscpy(nid.szInfo, message);
    nid.dwInfoFlags = NIIF_INFO;
    nid.uTimeout = 2000;

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void ShowPetStatus()
{
    wchar_t happinessStatus[50];
    if (myPet.happiness >= 90)
        wcscpy(happinessStatus, L"Your pet is happy");
    else
        wcscpy(happinessStatus, L"Your pet is sad");

    wchar_t status[256];
    swprintf(status, 256,
             L"%s\n\n"
             L"Hunger: %d/100\n"
             L"Happiness: %d/100\n"
             L"Energy: %d/100\n"
             L"Health: %d/100\n"
             L"State: %s\n"
             L"Mood: %s",
             myPet.name,
             myPet.hunger,
             myPet.happiness,
             myPet.energy,
             myPet.health,
             myPet.is_sleeping ? L"Sleeping" : L"Awake",
             happinessStatus);

    MessageBoxW(hMainWnd, status, L"Pet Status", MB_OK | MB_ICONINFORMATION);
}

void FeedPet()
{
    int oldHappiness = myPet.happiness;

    myPet.hunger = max(0, myPet.hunger - 5);

    if (myPet.energy < 100)
    {
        myPet.energy = min(100, myPet.energy + 5);
    }

    if (myPet.health < 100)
    {
        myPet.health = min(100, myPet.health + 5);
    }

    if (myPet.happiness < 100)
    {
        myPet.happiness = min(100, myPet.happiness + 1);
    }

    // Проверяем изменение состояния счастья после кормления
    CheckHappinessState(oldHappiness, myPet.happiness);

    if (oldHappiness < 100 && myPet.happiness == 100)
    {
        ShowNotification(L"Your pet is very happy", L"Your pet is extremely happy after being fed!");
    }
    else
    {
        ShowNotification(L"Feeding completed", L"Your pet has been fed and feels much better now!");
    }

    InvalidateRect(hMainWnd, NULL, TRUE);
}

void DrawPet(HDC hdc, RECT rect)
{
    int centerX = (rect.right - rect.left) / 2;
    int centerY = (rect.bottom - rect.top) / 2;

    HBRUSH bodyBrush;
    if (myPet.health > 70)      bodyBrush = CreateSolidBrush(RGB(0, 255, 0));
    else if (myPet.health > 30) bodyBrush = CreateSolidBrush(RGB(255, 255, 0));
    else                        bodyBrush = CreateSolidBrush(RGB(255, 0, 0));

    SelectObject(hdc, bodyBrush);
    Ellipse(hdc, centerX - 50, centerY - 30, centerX + 50, centerY + 30);

    HBRUSH headBrush = CreateSolidBrush(RGB(139, 69, 19));
    SelectObject(hdc, headBrush);
    Ellipse(hdc, centerX - 30, centerY - 60, centerX + 30, centerY - 10);

    HBRUSH eyeBrush = CreateSolidBrush(myPet.is_sleeping ? RGB(0, 0, 0) : RGB(255, 255, 255));
    SelectObject(hdc, eyeBrush);

    if (myPet.is_sleeping)
    {
        MoveToEx(hdc, centerX - 20, centerY - 40, NULL);
        LineTo(hdc, centerX - 10, centerY - 40);
        MoveToEx(hdc, centerX + 10, centerY - 40, NULL);
        LineTo(hdc, centerX + 20, centerY - 40);
    }
    else
    {
        Ellipse(hdc, centerX - 20, centerY - 45, centerX - 10, centerY - 35);
        Ellipse(hdc, centerX + 10, centerY - 45, centerX + 20, centerY - 35);
    }

    if (myPet.happiness >= 90 && !myPet.is_sleeping)
        Arc(hdc, centerX - 15, centerY - 30, centerX + 15, centerY - 15, centerX - 15, centerY - 22, centerX + 15, centerY - 22);
    else if (!myPet.is_sleeping)
        Arc(hdc, centerX - 15, centerY - 20, centerX + 15, centerY - 35, centerX - 15, centerY - 28, centerX + 15, centerY - 28);

    DeleteObject(bodyBrush);
    DeleteObject(headBrush);
    DeleteObject(eyeBrush);

    // Текст между изображением питомца и кнопкой "Feed"
    wchar_t moodText[50];
    if (myPet.happiness >= 90)
        wcscpy(moodText, L"Your pet is happy");
    else
        wcscpy(moodText, L"Your pet is sad");

    // Позиция текста - между изображением питомца и кнопкой
    TextOutW(hdc, centerX - 70, centerY + 80, moodText, (int)wcslen(moodText));
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rect;

    switch (message)
    {
        case WM_CREATE:
            hFeedButton = CreateWindowW(
                L"BUTTON",
                L"Feed",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                150, 350, 100, 30,
                hwnd,
                (HMENU)ID_BUTTON_FEED,
                ((LPCREATESTRUCTW)lParam)->hInstance,
                NULL);
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BUTTON_FEED)
            {
                FeedPet();
            }
            return 0;

        case WM_TIMER:
            UpdatePet();
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);
            DrawPet(hdc, rect);

            wchar_t stats[200];
            swprintf(stats, 200,
                     L"Hunger: %d   Happiness: %d   Energy: %d   Health: %d",
                     myPet.hunger, myPet.happiness, myPet.energy, myPet.health);
            TextOutW(hdc, 10, rect.bottom - 30, stats, (int)wcslen(stats));
            EndPaint(hwnd, &ps);

            return 0;

        case WM_PET_NOTIFICATION:
            if (lParam == WM_LBUTTONDBLCLK)
            {
                ShowPetStatus();
            }
            else if (lParam == WM_RBUTTONUP)
            {
                POINT pt;
                GetCursorPos(&pt);

                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, 1001, L"Feed");
                AppendMenuW(hMenu, MF_STRING, 1002, L"Play");
                AppendMenuW(hMenu, MF_STRING, 1003, L"Put to sleep");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, 1004, L"Status");
                AppendMenuW(hMenu, MF_STRING, 1005, L"Exit");

                SetForegroundWindow(hwnd);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);

                int oldHappiness = myPet.happiness;

                switch (cmd)
                {
                    case 1001:
                        myPet.hunger = max(0, myPet.hunger - 30);
                        myPet.happiness = min(100, myPet.happiness + 10);
                        CheckHappinessState(oldHappiness, myPet.happiness);
                        ShowNotification(L"Feeding time", L"Your pet has been fed and enjoyed the meal!");
                        break;

                    case 1002:
                        if (!myPet.is_sleeping)
                        {
                            myPet.happiness = min(100, myPet.happiness + 30);
                            myPet.energy = max(0, myPet.energy - 15);
                            CheckHappinessState(oldHappiness, myPet.happiness);
                            ShowNotification(L"Play time", L"Your pet is having fun playing with you!");
                        }
                        else ShowNotification(L"Pet is sleeping", L"Your pet is currently sleeping and cannot play!");
                        break;

                    case 1003:
                        myPet.is_sleeping = !myPet.is_sleeping;
                        ShowNotification(myPet.is_sleeping ? L"Good night" : L"Good morning",
                                        myPet.is_sleeping ? L"Your pet has gone to sleep" : L"Your pet woke up and is ready for the day");
                        break;

                    case 1004:
                        ShowPetStatus();
                        break;

                    case 1005:
                        PostQuitMessage(0);
                        break;
                }

                InvalidateRect(hwnd, NULL, TRUE);
            }

            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}
