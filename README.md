# OPPO / 一加 手机侧键增强模块

> 让三段式 / 新版侧键摆脱局限，解锁更多可能。

## 功能介绍

### 传统三段式按键

* **支持功能**：

  * 监听状态并执行自定义 Shell 脚本
  * 屏蔽官方操作（静音 / 震动 / 响铃）

### 新版自定义侧键

* **支持功能**：

  * 监听 **单击 + 长按 / 双击 + 长按** 操作，并执行自定义 Shell 脚本
  * ⚠️ **注**：单双击不可共存

## 使用方法

1. 刷入模块，并 **仔细阅读提示和教程**。
2. 三段式侧键：刷入后会自动屏蔽系统原有功能。
3. 新版自定义侧键：请手动设置为 **无操作**。
4. 模块安装包内提供示例 Shell：

   * 文件路径：`temp_shell.txt`
   * 可根据需求与安装提示进行自定义。

## 特色功能示例

* 一键开启 NFC
* 一键开关蓝牙
* 一键手电筒
* 一键录音
* 一键录屏
* 更多自定义操作

## 技术说明

* 模块使用 **C++ 编写**。
* 通过持续读取流进行监听。
* 将 **CPU 亲和设置为单小核**，功耗几乎可以忽略。

## Repo

![Alt](https://repobeats.axiom.co/api/embed/a156b5d3b81789c024ae5b796ca61e11048161f4.svg "Repobeats analytics image")

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=ItosEO/OplusKey&type=Date)](https://www.star-history.com/#ItosEO/OplusKey&Date)
