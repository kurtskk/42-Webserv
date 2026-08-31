*Este projeto foi criado como parte do currículo da 42 por jose-cad.*

---
# Webserv - Servidor HTTP em C++98

## Descrição

**Webserv** é um servidor HTTP/1.1 leve e não-bloqueante, implementado do zero em C++98. Este projeto demonstra conceitos centrais de programação de sistemas, incluindo programação de sockets, operações de I/O multiplexado, parsing de configuração e execução de scripts CGI.

O servidor é capaz de:
- Servir conteúdo estático e dinâmico
- Lidar com múltiplas conexões de clientes simultâneas sem threads
- Executar scripts CGI (Python, shell, etc.)
- Gerenciar uploads HTTP e operações de arquivo
- Processar métodos HTTP padrão: GET, POST, DELETE
- Redirecionar requisições conforme configurado
- Gerenciar cookies e sessões
- Suportar múltiplas configurações de servidor em portas diferentes

Construído inteiramente do zero, o Webserv ajuda a responder a pergunta fundamental: **"Por que toda URL começa com HTTP?"**

---

## Funcionalidades Principais

✅ **I/O Não-Bloqueante** - Uma única chamada `poll()` gerencia todos os descritores de arquivo  
✅ **Suporte Multi-Servidor** - Ouve em múltiplas portas simultaneamente  
✅ **Configuração Flexível** - Formato de arquivo `.conf` similar ao NGINX  
✅ **Métodos HTTP** - GET, POST, DELETE com restrições por localização  
✅ **Execução CGI** - Executa scripts com variáveis de ambiente corretas  
✅ **Tratamento de Erros** - Páginas de erro personalizáveis e códigos de status  
✅ **Upload de Arquivos** - Envio de arquivos de cliente com armazenamento configurável  
✅ **Cookies & Sessões** - Bônus: Suporte a gerenciamento de sessão e cookies  
✅ **Listagem de Diretório** - Autoindex de diretório configurável  
✅ **Redirecionamento de Requisição** - Redirecionamentos HTTP 301/302  

---

## Requisitos

- **Compilador C++**: g++ ou clang++ com suporte C++98
- **SO**: Linux ou macOS
- **Ferramenta de Build**: GNU Make
- **Bibliotecas**: Apenas bibliotecas padrão C/C++ (sem dependências externas)

### Flags de Compilação

```bash
-Wall -Wextra -Werror -std=c++98
```

---

## Instruções

### Compilando o Servidor

```bash
cd webserv
make clean
make
```

Isso gera o executável `webserv`.

### Estrutura do Projeto

```
.
├── Makefile                 # Configuração de build
├── webserv.conf             # Arquivo de configuração do servidor
├── README.md                # Este arquivo
│
├── includes/                # Arquivos de cabeçalho
│   ├── WebServ.hpp         # Classe principal do servidor (event loop baseado em poll)
│   ├── Client.hpp          # Handler de conexão de cliente
│   ├── HttpRequest.hpp     # Parser de requisição HTTP
│   ├── HttpResponse.hpp    # Gerador de resposta HTTP
│   ├── CgiHandler.hpp      # Execução de script CGI
│   ├── Config.hpp          # Parser de arquivo de configuração
│   ├── ServerConfig.hpp    # Configurações por servidor
│   ├── LocationConfig.hpp  # Configurações por rota
│   └── SocketUtils.hpp     # Utilitários de socket
│
├── srcs/                    # Arquivos fonte
│   ├── main.cpp            # Ponto de entrada
│   ├── server/
│   │   ├── WebServ.cpp     # Event loop & multiplexer
│   │   ├── Client.cpp      # Gerenciamento de cliente
│   │   └── SocketUtils.cpp # Operações de socket
│   ├── http/
│   │   ├── HttpRequest.cpp  # Parsing de requisição
│   │   ├── HttpResponse.cpp # Geração de resposta
│   │   └── CgiHandler.cpp   # Fork & execução CGI
│   └── config/
│       ├── Config.cpp       # Parser de configuração principal
│       ├── ServerConfig.cpp # Parsing de bloco server
│       └── LocationConfig.cpp # Parsing de bloco location
│
├── www/                     # Diretório raiz web
│   ├── index.html          # Página de índice padrão
│   ├── 404.html            # Página de erro não encontrado
│   ├── cgi-bin/            # Scripts CGI
│   │   ├── hello.py        # Script Python de demonstração
│   │   ├── session.py      # Demonstração de sessão/cookie
│   │   └── error_script.py # Teste de tratamento de erro
│   └── uploads/            # Diretório de upload de arquivo
│
└── YoupiBanane/             # Estrutura de diretório de teste
    ├── youpi.bla           # Arquivos de teste
    ├── youpi.bad_extension
    ├── nop/
    └── Yeah/
```

---

## Uso

### Executando o Servidor

```bash
./webserv webserv.conf
```

O servidor iniciará e exibirá:
```
Server listening on port 8080
Server listening on port 8081
WebServ is running...
```

Pressione `Ctrl+C` para encerrar graciosamente.

### Arquivo de Configuração (webserv.conf)

O arquivo de configuração define o comportamento do servidor usando sintaxe similar a NGINX:

```nginx
server {
    listen 8080;
    server_name localhost;
    
    root ./www;
    index index.html;
    
    error_page 404 /404.html;
    
    client_max_body_size 1000000;
    
    location / {
        allow_methods GET POST DELETE;
    }
    
    location /cgi-bin/ {
        allow_methods GET POST;
        cgi_ext .py;
        cgi_path /usr/bin/python3;
    }
    
    location /google {
        return 302 https://www.google.com;
    }
    
    location /uploads/ {
        allow_methods POST;
        upload_store ./www/uploads/;
    }
}
```

### Diretivas de Configuração

| Diretiva | Descrição | Exemplo |
|-----------|-----------|---------|
| `listen` | Número da porta | `listen 8080;` |
| `server_name` | Nome do host do servidor | `server_name localhost;` |
| `root` | Diretório raiz de documentos | `root ./www;` |
| `index` | Arquivo padrão para diretórios | `index index.html;` |
| `error_page` | Página de erro personalizada | `error_page 404 /404.html;` |
| `client_max_body_size` | Tamanho máximo de corpo de requisição | `client_max_body_size 1000000;` |
| `allow_methods` | Métodos HTTP permitidos | `allow_methods GET POST;` |
| `cgi_ext` | Extensão de script CGI | `cgi_ext .py;` |
| `cgi_path` | Caminho do interpretador CGI | `cgi_path /usr/bin/python3;` |
| `return` | Redirecionamento HTTP | `return 302 https://example.com;` |
| `upload_store` | Diretório de upload | `upload_store ./www/uploads/;` |
| `autoindex` | Listagem de diretório | `autoindex on;` |

---

## Testes

### Testes Rápidos

```bash
# Testar requisição GET básica
curl http://localhost:8080/

# Testar redirecionamento
curl -L http://localhost:8080/google

# Testar POST (upload de arquivo)
curl -X POST -F "file=@myfile.txt" http://localhost:8080/uploads/

# Testar DELETE
curl -X DELETE http://localhost:8080/testfile.txt

# Testar script CGI
curl http://localhost:8080/cgi-bin/hello.py

# Testar sessão/cookies
curl http://localhost:8080/cgi-bin/session.py
```

### Testes Avançados

**Usando telnet para HTTP bruto:**
```bash
telnet localhost 8080
GET / HTTP/1.1
Host: localhost
```

**Usando o navegador:**
- Navegue até `http://localhost:8080`
- Clique nos links de recursos de teste para validar recursos
- Faça upload de arquivos via formulário POST

---

## Conformidade com Especificação HTTP

O servidor implementa HTTP/1.1 conforme especificado em:
- **RFC 7230**: HTTP/1.1 Message Syntax and Routing
- **RFC 7231**: HTTP/1.1 Semantics and Content
- **RFC 7232**: HTTP/1.1 Conditional Requests
- **RFC 7233**: HTTP/1.1 Range Requests
- **RFC 3875**: Common Gateway Interface (CGI/1.1)

### Códigos de Status Suportados

| Código | Significado |
|--------|------------|
| 200 | OK |
| 201 | Created (Criado) |
| 301 | Moved Permanently (Movido Permanentemente) |
| 302 | Found (Encontrado - Redirecionamento Temporário) |
| 400 | Bad Request (Requisição Inválida) |
| 403 | Forbidden (Acesso Proibido) |
| 404 | Not Found (Não Encontrado) |
| 405 | Method Not Allowed (Método Não Permitido) |
| 413 | Payload Too Large (Payload Muito Grande) |
| 500 | Internal Server Error (Erro Interno do Servidor) |
| 504 | Gateway Timeout (Timeout do Gateway) |

### Métodos Suportados

- `GET` - Recuperar recursos
- `POST` - Enviar dados / fazer upload de arquivos
- `DELETE` - Remover recursos

---

## Arquitetura & Design

### Modelo Event-Driven Não-Bloqueante

Webserv usa uma única chamada `poll()` para multiplex I/O em todos os descritores de arquivo:

```
┌─────────────────────────────────┐
│      poll() Event Loop          │
├─────────────────────────────────┤
│                                 │
│  Server Sockets (Accept)        │
│  Client Sockets (Read/Write)    │
│  CGI Pipes (Output)             │
│                                 │
└─────────────────────────────────┘
     ↓                ↓              ↓
┌─────────┐  ┌──────────────┐  ┌────────┐
│ Accept  │  │ Read/Process │  │ CGI    │
│ Client  │  │ HTTP Request │  │ Output │
└─────────┘  └──────────────┘  └────────┘
```

**Por que poll()?** Ele monitora todos os descritores de arquivo em uma única chamada de sistema, eliminando a sobrecarga de verificações por descritor e habilitando verdadeira operação não-bloqueante.

### Divisão de Módulos

**Módulo Config**
- Faz parsing de `webserv.conf` usando tokenizador customizado
- Valida diretivas e constrói configuração hierárquica
- Suporta blocos server/location aninhados
- Retorna objetos `ServerConfig` estruturados

**Módulo HTTP**
- Faz parsing de requisições HTTP brutas de buffers de socket
- Gera headers de resposta HTTP corretos
- Manipula execução CGI e saída de script
- Gerencia dados de cookies e sessão
- Implementa todos os códigos de status e páginas de erro

**Módulo Server**
- Implementa event loop baseado em poll()
- Gerencia criação e binding de socket
- Manipula ciclo de vida de conexão de cliente
- Encanamentos de saída CGI de volta para clientes
- Implementa shutdown gracioso

---

## Decisões Técnicas

### 1. poll() Sobre select()

**Escolha:** `poll()` para multiplexing  
**Justificativa:** 
- Sem limite de descritor de arquivo (FD_SETSIZE)
- Interface mais limpa para gerenciar muitos sockets
- Melhor performance com conjuntos de descritores de arquivo esparsos

### 2. Rastreamento de Processo CGI Baseado em Map

**Escolha:** `std::map<int, CgiProcess>` para rastrear pipes  
**Justificativa:**
- Lookup O(log n) por fd de pipe
- Limpeza automática no término do processo
- Compatível com C++98

### 3. Matching de Caminho Mais Longo para Locations

**Escolha:** Iterar todas as locations, fazer match do caminho mais longo  
**Justificativa:**
- Corresponde ao comportamento do NGINX
- Simples de entender e debugar
- Performance suficiente para configurações típicas

### 4. Fork/Exec para CGI (Sem Thread Pool)

**Escolha:** Fork processo por requisição CGI  
**Justificativa:**
- Segue a filosofia Unix (um processo por tarefa)
- Gerenciamento simples do ciclo de vida do processo
- Evita complexidade de threading
- Separação limpa entre parent/child

### 5. Global Sessions Map

**Escolha:** Static `std::map` em HttpResponse  
**Justificativa:**
- Dados de sessão sobrevivem através de requisições HTTP
- Acesso simples sem chains de ponteiros complexas
- Aceitável para servidor single-threaded

---

## Recursos Bônus

Além da especificação necessária, esta implementação inclui:

✨ **Suporte a Cookies**
- Parsing automático de cookies do header `Cookie:`
- Geração de header `Set-Cookie:`
- Persistência de dados de sessão

✨ **Gerenciamento de Sessão**
- Geração automática de ID de sessão (UUID)
- Armazenamento de sessão no servidor
- Persistência de sessão em múltiplas requisições

✨ **Testes de Tratamento de Erro**
- Scripts de demonstração para timeout (infinite_loop.py)
- Recuperação de erro (error_script.py)
- Automação de teste abrangente

---

## Solução de Problemas

### Porta Já em Uso
```bash
# Matar qualquer processo usando a porta 8080
lsof -i :8080 | grep -v PID | awk '{print $2}' | xargs kill -9

# Ou usar fuser
fuser -k 8080/tcp
```

### Script CGI Não Executando
- Verifique se `cgi_path` aponta para um interpretador válido
- Verifique permissões do script: `chmod +x script.py`
- Verifique se `cgi_ext` corresponde à extensão do script
- Verifique saída CGI: `curl http://localhost:8080/cgi-bin/hello.py`

### Erros de Parsing de Configuração
- Valide sintaxe webserv.conf (ponto-e-vírgula e chaves faltando)
- Verifique se caminhos de arquivo existem (root, cgi_path, upload_store)
- Verifique se não há diretivas listen conflitantes

---

## Recursos

### Documentação e Referências Clássicas

**Especificações HTTP**
- [RFC 7230 - HTTP/1.1 Message Syntax](https://tools.ietf.org/html/rfc7230)
- [RFC 7231 - HTTP/1.1 Semantics](https://tools.ietf.org/html/rfc7231)
- [RFC 3875 - CGI/1.1](https://tools.ietf.org/html/rfc3875)

**Programação de Sistemas**
- [poll() man page](https://man7.org/linux/man-pages/man2/poll.2.html)
- [socket() man page](https://man7.org/linux/man-pages/man2/socket.2.html)
- [fork() & execve() guide](https://man7.org/linux/man-pages/man2/execve.2.html)

**Comparação & Testes**
- [Documentação Oficial NGINX](https://nginx.org/en/docs/)
- Testadores oficiais do subject (`cgi_tester`, `tester`) — usados durante o desenvolvimento para comparar comportamento com a referência da escola

### Como a IA Foi Usada Neste Projeto

Este projeto utilizou assistência de IA para:

1. **Consultoria Técnica** - Interpretação de RFC, decisões de compatibilidade C++98, padrões de arquitetura poll()
2. **Geração de Código** - Estrutura inicial para parser de config, handlers de requisição/resposta HTTP, utilitários de socket
3. **Documentação** - Matrizes de conformidade RFC, diagramas de arquitetura, guias técnicos para cada módulo
4. **Debugging** - Análise de comportamento poll(), setup de variáveis de ambiente CGI, formatação de headers HTTP
5. **Automação de Testes** - Criação de scripts de teste abrangentes, scripts CGI de demonstração, verificação de recursos
6. **Code Review** - Verificação de conformidade C++98, correção de I/O não-bloqueante, gerenciamento de memória

A IA serviu como um consultor técnico e gerador de testes/frontend, enquanto os desenvolvedores mantiveram compreensão total e controle da implementação final.

---

## Licença

Projeto educacional para o currículo 42 da escola.

---

**"É aqui que você finalmente entende por que URLs começam com HTTP"** ✨
