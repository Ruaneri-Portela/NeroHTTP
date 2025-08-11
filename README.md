# 🔥 Nero HTTP

**English** | **Português**

---

# 🇺🇸 Nero HTTP

**Nero HTTP** is an HTTP/HTTPS server written in **C**, focused on learning, modularity, and simplicity.  
Perfect for those who want to understand how a web server works in practice — from handling connections to serving files and encrypting with OpenSSL.

## ✨ Features

- ✅ Lightweight DIY HTTP(S) server
- 📁 Support for file listing and download
- 🧩 Modular structure for easy extension and maintenance
- 📄 Customizable default page
- 🔐 Encryption via **OpenSSL**
- 🧱 Cross-platform build with **CMake**
- 💡 Ideal for learning, POCs, or simple local servers

## 💻 Platform Support

| Operating System    | Support | Notes         |
|---------------------|---------|--------------|
| 🐧 **Linux**        | ✅ Full |              |
| 🪟 **Windows**      | ✅ Full | via MSYS2    |
| 🍎 **macOS**        | ✅ Full | via Homebrew |

## 📦 Requirements

- **CMake** (3.10+)
- **C Compiler** (GCC/Clang/MSVC)
- **OpenSSL** (1.1 or higher)

---

## 🚀 Build

```bash
git clone https://github.com/Ruaneri-Portela/nero-http.git
cd nero-http
mkdir build && cd build
cmake ..
make
```

---

## 🔧 Run

```bash
./nero-http
```

By default, the server listens on port `9000` with HTTPS.  
Make sure to generate your `cert.pem`!  
It will look for the `root` folder in the working directory.

---

## 📂 Example Navigation

```
http://localhost:9000/
```

You will see a dynamically generated file listing with support for:

- 📥 Direct download
- 📄 File preview (when applicable)
- 📁 Subfolder navigation

## 🤝 Contributing

Contributions are welcome! Feel free to open issues, submit pull requests, or suggest improvements.

## 📧 Contact

Developed by **Ruaneri Ferreira Portela**  
[📫 ruaneriportela@outlook.com]  
[🔗 LinkedIn](https://www.linkedin.com/in/ruaneri-portela-aa6945227/)

---

# 🇧🇷 Nero HTTP

**Nero HTTP** é um servidor HTTP/HTTPS escrito em **C**, com foco em aprendizado, modularidade e simplicidade.  
Ideal para quem deseja entender como funciona um servidor web na prática — desde a recepção de conexões até o envio de arquivos e criptografia com OpenSSL.

## ✨ Recursos

- ✅ Servidor HTTP(S) leve e DIY
- 📁 Suporte à listagem e download de arquivos
- 🧩 Estrutura modular para fácil extensão e manutenção
- 📄 Página padrão personalizável
- 🔐 Criptografia via **OpenSSL**
- 🧱 Compilação multiplataforma com **CMake**
- 💡 Ideal para estudos, POCs ou servidores locais simples

## 💻 Suporte a Plataformas

| Sistema Operacional | Suporte | Observações |
|---------------------|---------|-------------|
| 🐧 **Linux**        | ✅ Total |              |
| 🪟 **Windows**      | ✅ Total | Via MSYS2    |
| 🍎 **macOS**        | ✅ Total | Via homebrew |

## 📦 Requisitos

- **CMake** (3.10+)
- **Compilador C** (GCC/Clang/MSVC)
- **OpenSSL** (1.1 ou superior)

---

## 🚀 Compilação

```bash
git clone https://github.com/Ruaneri-Portela/nero-http.git
cd nero-http
mkdir build && cd build
cmake ..
make
```

---

## 🔧 Execução

```bash
./nero-http
```

Por padrão, o servidor escuta na porta `9000` com HTTPS.  
Lembre-se de gerar o `cert.pem`!  
Vai procurar a pasta `root` na pasta de trabalho.

---

## 📂 Exemplo de Navegação

```
http://localhost:9000/
```

Você verá uma listagem de arquivos servidos dinamicamente com suporte a:

- 📥 Download direto
- 📄 Visualização de arquivos (quando aplicável)
- 📁 Navegação por subpastas

## 🤝 Contribuindo

Contribuições são muito bem-vindas! Sinta-se à vontade para abrir *issues*, enviar *pull requests* ou sugerir melhorias.

## 📧 Contato

Desenvolvido por **Ruaneri Ferreira Portela**  
[📫 ruaneriportela@outlook.com]  
[🔗 LinkedIn](https://www.linkedin.com/in/ruaneri-portela-aa6945227/)
