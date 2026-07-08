#!/usr/bin/env python3
# Script de teste: Loop infinito para validar timeout do servidor (120s)
# O servidor deve interromper este script após 120 segundos e retornar 504 Gateway Timeout

import sys

print("Content-Type: text/plain\r")
print("\r")
print("Starting infinite loop... Server should timeout after 120 seconds")
sys.stdout.flush()

# Loop infinito - servidor vai matar após 120s
while True:
    pass
