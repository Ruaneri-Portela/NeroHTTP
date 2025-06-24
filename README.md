# 🔥 Nero HTTP

**Nero HTTP** é um servidor HTTP/HTTPS escrito em **C**, com foco em aprendizado, modularidade e simplicidade. Ideal para quem deseja entender como funciona um servidor web na prática — desde a recepção de conexões até o envio de arquivos e criptografia com OpenSSL.


## ✨ Recursos

- ✅ Servidor HTTP(S) leve e DIY
- 📁 Suporte à listagem e download de arquivos
- 🧩 Estrutura modular para fácil extensão e manutenção
- 📄 Página padrão personalizável
- 🔐 Criptografia via **OpenSSL**
- 🧱 Compilação multiplataforma com **CMake**
- 💡 Ideal para estudos, POCs ou servidores locais simples


## 💻 Suporte a Plataformas

O **Nero HTTP** é compatível com os principais sistemas operacionais:

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

Por padrão, o servidor escuta na porta `9000` com https, lembrando gere o cert.pem ! Vai procurar a pasta root na pasta de trabalho

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
[🔗 [LinkedIn](https://www.linkedin.com/in/ruaneri-portela-aa6945227/)]