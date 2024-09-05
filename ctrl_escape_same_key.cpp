// Upon tapping lcontrol, so long as no other keys are pressed between press and release, we simulate a keypress and release of escape.
// Upon holding lcontrol and tapping another key, we simulate a combination of control + that other key.
// tl;dr puts escape/lcontrol on the same key.
#include <iostream>

#include <Windows.h>

HHOOK g_keyboard_hook;

struct Context {
  bool is_ctrl_key_pressed = false;
  bool other_key_involved = false;
}
g_context;

// This will get flagged by most anticheats. 
void sim_key_down(WORD vkCode) {
  INPUT input = {
    0
  };
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = vkCode;
  SendInput(1, & input, sizeof(INPUT));
}

void sim_key_up(WORD vkCode) {
  INPUT input = {
    0
  };
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = vkCode;
  input.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, & input, sizeof(INPUT));
}

// Because Windows overcomplicated input processing, you cannot just set pKeyboard->vkCode/scanCode to the key you want to simulate.
// You have to use SendInput or keybd_event etc.
// ---------------------------------------------------------------------------------------------------------------------------------
LRESULT CALLBACK hook_callback(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HC_ACTION) {
    KBDLLHOOKSTRUCT * pKeyboard = (KBDLLHOOKSTRUCT * ) lParam;

    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
      if (pKeyboard -> vkCode == VK_LCONTROL) {
        g_context.is_ctrl_key_pressed = true;
        return 1;
      } else if (g_context.is_ctrl_key_pressed) {
        g_context.other_key_involved = true;
        UnhookWindowsHookEx(g_keyboard_hook);
        sim_key_down(VK_LCONTROL);
        sim_key_down(pKeyboard -> vkCode);
        sim_key_up(pKeyboard -> vkCode);
        sim_key_up(VK_LCONTROL);
        g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, hook_callback, NULL, 0);
        return 1;
      }
    }

    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
      if (pKeyboard -> vkCode == VK_LCONTROL) {
        g_context.is_ctrl_key_pressed = false;
        if (!g_context.other_key_involved) {
          sim_key_down(VK_ESCAPE);
          sim_key_up(VK_ESCAPE);
        }
        g_context.other_key_involved = false;
        return 1;
      }
    }
  }

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void install_hook() {
  g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, hook_callback, NULL, 0);
  if (g_keyboard_hook == NULL) {
    printf("Failed to set hook: %d\n", GetLastError());
  } else {
    printf("Hook set\n");
  }
}

int main() {
  install_hook();

  // keep-alive
  MSG msg;
  while (GetMessage( & msg, NULL, 0, 0)) {
    TranslateMessage( & msg);
    DispatchMessage( & msg);
  }

  UnhookWindowsHookEx(g_keyboard_hook);
}
