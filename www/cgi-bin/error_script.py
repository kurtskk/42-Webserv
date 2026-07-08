#!/usr/bin/env python3
# Script de teste: Erro no CGI para validar tratamento de falhas
# O servidor deve detectar o erro e retornar 500 Internal Server Error

import sys

print("Content-Type: text/plain\r")
print("\r")
print("This script will cause an error")
sys.stdout.flush()

# Tentar acessar uma variável não definida (NameError)
print(undefined_variable)
