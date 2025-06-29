<h1>Acender led do ESP32 usando por meio de cliente MQTT via wi-fi</h1>

<i>Este projeto implementa um cliente MQTT que se conecta a um broker, e se inscreve no tópico específicado na atividade `/ifpe/ads/embarcados/esp32/led` e controla o estado do LED embutido na placa <b>ESP32</b> no <b>GPIO 2</b> com base nas mensagens recebidas do cliente inscrito no tópico ("1" para ligar, "0" para desligar).</i>

Inicialmente, é necessário que você tenha o ESP-IDF instalado em seu VScode para rodar o código.

Foi utilizado para realizar o teste de funcionamento desse projeto como Broker local o <b>Mosquitto</b> e o <b>MQTTX</b> como cliente para enviar a mensagem.

<h1>Instruções para rodar o projeto na sua máquina usando o ESP-IDF no VScode, o broker Mosquitto e o cliente MQTTX.</h1> 

<h2>Instalação de broker e cliente</h2>
<h3>Broker Mosquitto:</h3>

Faça o download e instale o broker <b>Mosquitto</b> em sua máquina https://mosquitto.org/download/

1. Realize o download e a instalação do Mosquitto compatível com seu sistema operacional. 
2. Siga as instruções do instalador, utilizando as opções padrão.
3. Navegue até o diretório de instalação do Mosquitto e edite o arquivo de configuração mosquitto.conf com um editor de texto.
4. Adicione as seguintes linhas ao final do arquivo para habilitar o listener na porta padrão e permitir conexões anônimas. Salve o arquivo.

```
listener 1883
allow_anonymous true
```

4. Abra um terminal `(como administrador no Windows)` e navegue até o diretório de instalação do Mosquitto.
5. Inicie o broker com o seguinte comando para carregar a configuração e exibir logs de conexão no console:
```
mosquitto -c mosquitto.conf -v
```
> <b>Observação:</b> No Windows pode ocorrer problema com o firewall relacionado a porta que o Broker irá utilizar, para habilitar essa permissão faça uma regra de firewall como a descrita abaixo.
- Aperte os botões `Windows + R` simultaneamente e digite `wf.msc`;
- Clique em `Regras de Entrada`, ao lado clique em `Nova Regra`;
- Selecione a opção `Porta` e avançar;
- Na próxima tela deixe a opção `TCP` marcada, e na parte <b>"Portas locais específicas"</b>, digite a porta do Broker por padrão é `1883` e clique em avançar;
- Agora deixe na opção `Permitir a conexão` e avançar;
- Nessa nova etapa pode deixar as duas primeiras opções marcadas e avançar;
- Para finalizar dê um nome para a regra `(Ex: Mosquitto 1883)` e aperte em concluir.
  
<h3>Cliente MQTTX:</h3>
Da mesma forma baixe e instale o cliente <b>MQTTX</b> https://mqttx.app/downloads.

- Realize o download e a instalação do <b>MQTTX</b>. A instalação é direta e segue o padrão do seu sistema operacional.


<h2>Configurando o projeto para suas credenciais</h2>
Abra o VScode e a extensão <b>ESP-IDF</b>, faça um clone desse projeto e importe no <b>ESP-IDF</b>.

Agora com o projeto aberto no <b>ESP-IDF</b>, iremos realizar as alterações abaixo:

- No arquivo `sdkconfig`:

Na linha `412`, configuramos o broker:
```
CONFIG_BROKER_URL="mqtt://mqtt.eclipseprojects.io"
```
Onde tem `mqtt.eclipseprojects.io`, você deve modificar para o endereço <b>IP</b> do seu pc, caso não saiba o endereço IP, abra o cmd do computador (se for Windows) e digite `ipconfig`, e pegue o endereço <b>IPV4</b>.

Nas linhas `425` e `426` iremos configurar a nossa rede que o <b>ESP32</b> irá se conectar:
```
CONFIG_EXAMPLE_WIFI_SSID="myssid"
CONFIG_EXAMPLE_WIFI_PASSWORD="mypassword"
```

No lugar de `myssid` coloque o nome do seu wi-fi `(Utilizar a rede wi-fi da frequência de 2.4)`, e no lugar de `mypassword` coloque a senha do seu wi-fi.


Após isso, com o <b>ESP32</b> conectado no computador via USB. aperte no ESP-IDF no ícone de 'chama' (build, flash e monitor), para mandar o código para a placa e rodar o projeto.

- Na interface do VSCode, utilize a funcionalidade "Build, Flash, and Monitor" da extensão ESP-IDF (representada por um ícone de chama).

Este processo irá compilar o projeto, gravar o firmware na placa e iniciar o monitor serial, onde você poderá acompanhar os logs de execução.


<h2>Conectando o MQTTX no broker e adicionando o tópico para enviar a mensagem para o ESP32</h2>

1. Abra o programa MQTTX, e clique em `new connection`.
2. Configure a conexão:
   
   - Name: Forneça um nome descritivo (ex: Teste LED ESP32).
   - Host: Selecione `mqtt://` e insira o mesmo endereço IP do broker configurado no projeto.
   - Mantenha as demais configurações padrão e clique em `Connect`.

3. Com a conexão estabelecida, configure a mensagem a ser enviada:
   - Na seção de publicação na parte inferior direita, defina o formato do payload para `Plaintext`.
   - Defina o QoS como `1`.
   - No campo <b>Topic</b>, insira o tópico exato definido no projeto: `/ifpe/ads/embarcados/esp32/led`.
   - Na área de texto do <b>payload</b>, digite `1` para ligar o LED ou `0` para desligar.
     
4. Clique no botão de envio (ícone verde de avião de papel). O LED na sua placa ESP32 deverá mudar de estado instantaneamente. Os logs da operação podem ser visualizados no monitor serial do ESP-IDF.
