## 更新日志

### 1.3.1

添加基于 v1.3.0 的 steam 启动方式，同时允许绕过 secure 限制。

### 1.3.0

添加 kpatch.ini，允许自定义大小；尝试性的修正 dynamic vertex buffer 大小
> 注意，使用过量 mod、高面数/高精细的模型或者复杂粒子特效 or 光照仍然可能导致内存溢出，或者 CallQueue 溢出，
> 建议自行微调 `-heapsize`，实测不能过低也不能太高，太高会也会导致闪退。 我一般建议在 512M(524288) -> 2G(2097152) 之间找一个稳定的值，我测试的时候一般用 1.5G。
> 可以配合 dxvk 食用补丁（_fix.exe 同级放 32 位的 d3d9.dll + 可选 dxgi.dll ），建议不要用 -vulkan 启动项。

### 1.2.0

抛弃 proxy dll 方法，改用 Launcher 启动。

### 1.1.1

使用 winhttp.dll 作为 loader，避免 version.dll 不稳定情况。

### 1.1.0

添加对 `-Vulkan` 启动项的支持。
