Trabalho de Programação - Redes II
Chat Cliente-Servidor
-----------------------------------------

1- Compile o código fonte do cliente e do servidor junto com a flag pthread.
	Exemplo: gcc -pthread cliente.c -o cliente
2- Execute primeiramente o Servidor. Você pode passar como argumento a porta 
de escuta
	Exemplo: ./servidor 5678
   No exemplo acima, o servidor subirá na porta 5678. Caso não passe nenhuma
porta como argumento, o servidor vai subir na porta 2017 (padrão).

3- Ao executar a aplicação cliente.
	Exemplo: ./cliente {ip} {porta}
   Podendo inserir somente o ip do servidor como argumento e opcionalmente a
porta de escuta do servidor.
   Os argumentos são opcionais, a aplicação tentará se conectar na interface
loopback (caso o servidor rode na maquina local) e na porta 2017.

-----------------------------------------