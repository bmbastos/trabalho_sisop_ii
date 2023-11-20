# Drop Box (<s>da Shopee</s>)

> **Abertura**: 5 de novembro de 2023 (05/11/23) - 00h00min

> **Fechamento**: 4 de dezembro de 2023 (04/12/23) - 13h30min 

### Integrantes:
* Bruno Marques Bastos (314518)
* Eduardo (313439)
* Marcel (314937)
* Vinícius (330087)

## Trabalho da disciplina de Sistemas Operacionais 2
Este projeto consiste na implementação de um serviço semelhante ao Dropbox, para permitir o compartilhamento e a sincronização automática de arquivos entre diferentes dispositivos de um mesmo usuário. O trabalho está dividido em duas partes. A primeira parte compreende tópicos como: threads, processos, comunicação utilizando sockets e sincronização de processos utilizando mutex e semáforos. Posteriormente, novas funcionalidades serão adicionadas ao projeto. A aplicação deverá executar emambientes Unix (Linux), mesmo que tenha sido desenvolvida em outras plataformas. O programa deverá ser implementado utilizando a API Transmission Control Protocol (TCP) sockets do Unix e utilizando C/C++.

## Funcionalidades Básicas
1) **Múltiplos usuários**: O servidor deve ser capaz de tratar requisições simultâneas de vários usuários. 

2) **Múltiplas sessões**: Um usuário deve poder utilizar o serviço através de até dois dispositivos distintos simultaneamente (Assumindo que o mesmo usuário com dois dispositivos/terminais abertos simultaneamente, não irá editar o mesmo arquivo simultaneamente)

3) **Consistência nas estruturas de armazenamento**: As estruturas de armazenamento de dados no servidor devem ser mantidas em um estado consistente e protegidas de acessos concorrentes.

4) **Sincronização**:  Cada vez que um usuário modificar um arquivo contido no diretório ‘sync_dir’ em seu dispositivo, o arquivo deverá ser atualizado no servidor e no diretório ‘sync_dir’ dos demais dispositivos daquele usuário.

5) **Persistência de dados no servidor**: Diretórios e arquivos de usuários devem ser restabelecidos quando o servidor for reiniciado.
