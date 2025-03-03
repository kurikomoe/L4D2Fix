[![Github All Releases](https://img.shields.io/github/downloads/kurikomoe/L4D2Fix/total.svg)]()

### L4D2 Too Many Indices Fix

用来处理这个问题：

![26cc8e80ba807d378291266749f824bc](https://github.com/user-attachments/assets/9a6aed1c-ef76-43a0-b9bb-dd357f0b9261)


### 编译
```
just build
```

### 原理解析：

> 虽然一开始是靠逆向找的，后来发现自己犯傻了，明明 source 引擎是开源了部分内容的，直接看源代码不香么 QAQ

逆向的话，基本上就是定位字符串，之后 trace 追一下代码的执行流（或者直接搜索立即数 0x8000，反正也没几个结果，之后猜一下函数是干什么的）



从源代码更简单，直接看就行了。

```c++
enum
{
	VERTEX_BUFFER_SIZE = 32768,  // 罪魁祸首，你开这么小的 buffer 干什么
	MAX_QUAD_INDICES = 16384,
};
```

位置： https://github.com/nillerusr/source-engine/blob/29985681a18508e78dc79ad863952f830be237b6/materialsystem/shaderapidx9/meshdx8.cpp#L71



对比 dx10：（32倍差距）

```c++
VERTEX_BUFFER_SIZE = 1024 * 1024
```


### 修正方法

一开始是直接 patch 立即数，但是 L4D2 更新有点频繁，就索性直接写成动态 patch 了。
