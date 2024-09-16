# Dectetor de Enchentes IoT
Projeto desenvolvido para o curso de graduação em Engenharia Elétrica no Instituto Federal da Paraíba.
O sistema consiste de:
- sensor de distância ultrassônico hc sr04
- módulo Wifi LoRa 32 V2 da Heltec
- LED
- Buzzer

## Arquitetura
O sistema utiliza **FreeRTOS** para gerenciar as tarefas de leitura do sensor e processar os dados, realizar alerta sonoro e visual e se comunicar via **MQTT** com uma aplicação de monitoramento remoto feita em **Python** utilizando as bibliotecas **Custom Tkinter** e **Paho MQTT**. Futuramente, será adicionado outro módulo Wifi LoRa 32,
havendo um detector remoto que ficará em campo realizando as medições e enviando os dados via **LoRa** e um receptor que estará conectado a internet, para direcionar os dados via MQTT.
Outro upgrade futuro será a adição de um sistema de alimentação com Painel Solar, baterias e bms para tornar o detector Autônomo.

## Testes
- Consumo do dispositivo
- Range máximo de operação
- Taxa de transmissão
