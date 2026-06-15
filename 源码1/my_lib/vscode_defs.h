#ifndef _VSCODE_DEFS_H_
#define _VSCODE_DEFS_H_

// 仅在未定义时，补充STM32核心宏（解决VSCode语法误报）
#ifndef STM32F10X_MD
#define STM32F10X_MD
#define USE_STDPERIPH_DRIVER
#endif

#endif // _VSCODE_DEFS_H_

