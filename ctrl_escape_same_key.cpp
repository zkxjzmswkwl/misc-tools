// Upon tapping lcontrol, we simulate a keypress and release of escape.
// Upon holding lcontrol, we simulate a keydown of lcontrol.
// Upon releasing lcontrol after holding it, we simulate a keyup of lcontrol.
// tl;dr puts escape/lcontrol on the same key.
#include <iostream>

#include <Windows.h>

HHOOK g_keyboard_hook;

struct Context {
  unsigned int ctrl_keypress_duration = 0;
  bool is_ctrl_key_sim_down = false;
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
      // if lcontrol is pressed down
      if (pKeyboard -> vkCode == 162) {
        // increment some arbitrary representation of how long that key has been pressed for.
        g_context.ctrl_keypress_duration++;
        // check if it's > 1 and if we've not already simulated a downward press for lcontrol.
        if (g_context.ctrl_keypress_duration > 1 && !g_context.is_ctrl_key_sim_down) {
          // synthetic input for key down.
          sim_key_down(VK_LCONTROL);
          // flip to check against above as to avoid spam.
          g_context.is_ctrl_key_sim_down = true;
        }
        // returning non-zero in this callback blocks the input.
        return 1;
      }
    }

    // if lcontrol is released
    if (wParam == WM_KEYUP && pKeyboard -> vkCode == 162) {
      // if the key was pressed for less than 1 <arbitrary unit of time>
      if (g_context.ctrl_keypress_duration <= 1) {
        // synthetic input for escape key down and release.
        sim_key_down(VK_ESCAPE);
        sim_key_up(VK_ESCAPE);
        g_context.ctrl_keypress_duration = 0;
        // returning non-zero in this callback blocks the input.
        return 1;
      } else {
        // If we held down lcontrol for long enough, it means we want to actually input lcontrol.
        sim_key_up(VK_LCONTROL);
        // reset state
        g_context.ctrl_keypress_duration = 0;
        g_context.is_ctrl_key_sim_down = false;
        // returning non-zero in this callback blocks the input.
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
