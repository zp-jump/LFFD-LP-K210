# Kmodel 模型的生成与测试

使用 ncc 对模型进行转换，生成 `*.kmodel` 文件

## 命令
```cmd
.\ncc.exe compile  .\LPbox.tflite .\LPbox.kmodel -i tflite --dataset .\dataset\  --input-std 0.0039215686274509803921568627451
.\ncc.exe infer .\LPbox.kmodel .\out\ --dataset .\dataset\ --input-std 0.0039215686274509803921568627451
```

## 注意事项

ncc 的默认模型输入范围为 0 ~ 1 其本质是将标准8位像素除以255，可以通过更改选项 `--input-std` 与 `--input-mean` 选项进行更改。ncc 在处理输入时会将输入 in 进行以下处理：$\frac{input - mean}{std}$ 。所以对于输入模型范围是 0 ~ 255 的模型要将 `--input-mean` 设置为 $\frac{1}{250}$ 即 0.0039215686274509803921568627451。 