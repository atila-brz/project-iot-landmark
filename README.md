# Projeto IoT: Controle de Dispositivos com Reconhecimento Facial e Dashboard

Este projeto demonstra uma solução completa de Internet das Coisas (IoT) que permite o controle de dispositivos eletrônicos por meio de gestos faciais, além de monitorar e visualizar dados de telemetria em um dashboard web.

## Arquitetura e Fluxo de Funcionamento

O sistema é dividido em quatro módulos principais que interagem entre si para criar a solução completa.

**Fluxo de Controle:**

1.  **Frontend (MediaPipe)**: O usuário acessa uma página HTML (`MediaPipe/holistic.html`) que utiliza a webcam e a biblioteca MediaPipe do Google para detectar marcos faciais em tempo real no navegador.
2.  **Detecção de Gesto**: Um código JavaScript (`MediaPipe/js/holistic.js`) analisa esses marcos para identificar gestos específicos (ex: boca aberta, piscar de olho).
3.  **Requisição à API**: Ao detectar um gesto, o JavaScript envia uma requisição HTTP (POST) para o endpoint da API Python, informando o comando a ser executado.
4.  **Backend (API)**: O servidor Flask/FastAPI (`API/app.py`) recebe a requisição, a processa e a retransmite para o microcontrolador (ESP) que está na mesma rede.
5.  **Ação no Hardware (ESP)**: O microcontrolador (`CodigoEsp/controle-facial.ino`), que está constantemente escutando por comandos da API, recebe a instrução e executa a ação física correspondente, como acender um LED ou mover um servo motor.

**Fluxo de Telemetria:**

1.  **Envio de Dados (ESP)**: O microcontrolador pode, periodicamente ou quando um evento ocorre, enviar dados de sensores (temperatura, umidade, status, etc.) para um endpoint específico da API.
2.  **Armazenamento (API)**: A API recebe esses dados e os armazena no banco de dados SQLite (`API/telemetria.db`).
3.  **Visualização (Dashboard)**: As páginas do dashboard (`Dashboard/dashboard.html` e `graficos.html`) fazem requisições periódicas à API para obter os dados de telemetria mais recentes.
4.  **Exibição de Dados**: O JavaScript no dashboard processa os dados recebidos e os exibe em formato de painéis, tabelas e gráficos.

## Componentes do Projeto

### 1. `API`

O backend do projeto, construído em Python.
- **`app.py`**: O servidor web (provavelmente Flask ou FastAPI) que atua como ponte entre o frontend, o hardware e o banco de dados. Ele expõe endpoints para receber comandos e para fornecer dados de telemetria.
- **`requeriments.txt`**: Lista de dependências Python necessárias para executar o servidor.
- **`telemetria.db`**: Banco de dados SQLite onde os dados de telemetria são armazenados.

### 2. `CodigoEsp`

O firmware para o microcontrolador (ESP32 ou ESP8266).
- **`controle-facial.ino`**: Código em C++/Arduino que se conecta à rede Wi-Fi, configura um servidor web ou cliente para escutar por comandos vindos da API e controla os pinos GPIO para interagir com atuadores (LEDs, relés, etc.).

### 3. `MediaPipe`

A interface de frontend para o controle por gestos.
- **`face.html`, `hands.html`, etc.**: Páginas web que implementam diferentes modelos do MediaPipe para detecção de partes do corpo. O arquivo principal para este projeto é o `holistic.html`.
- **`js/`**: Contém os arquivos JavaScript correspondentes que gerenciam a lógica da câmera, detecção de gestos e comunicação com a API.
- **`img/`**: Imagens de exemplo ou assets para as páginas web.

### 4. `Dashboard`

O frontend para visualização dos dados coletados.
- **`dashboard.html` e `graficos.html`**: Páginas web estáticas que utilizam JavaScript para buscar e exibir os dados de telemetria da API de forma amigável.

## Como Executar o Projeto

Siga os passos abaixo para executar cada módulo do sistema.

### Pré-requisitos

- Python 3.7+ e `pip` instalados.
- Arduino IDE com o Board Manager para ESP32 ou ESP8266 instalado.
- Um navegador web moderno (Chrome, Firefox).
- O microcontrolador ESP e os componentes de hardware (LEDs, etc.).

### 1. Hardware (ESP)

1.  Abra o arquivo `CodigoEsp/controle-facial.ino` na Arduino IDE.
2.  Instale as bibliotecas necessárias (ex: `WiFi.h`, `HTTPClient.h`, etc.) através do Library Manager.
3.  Modifique as variáveis no código para incluir o **SSID** e a **senha** da sua rede Wi-Fi.
4.  Altere o endereço IP no código para apontar para o IP da máquina onde a API Python estará rodando.
5.  Conecte o microcontrolador ao seu computador, selecione a porta e a placa corretas na IDE e faça o upload do código.
6.  Abra o Serial Monitor para verificar se a conexão com o Wi-Fi foi bem-sucedida e para ver o endereço IP atribuído ao ESP.

### 2. Backend (API)

1.  Abra um terminal e navegue até a pasta `API`.
2.  (Opcional, mas recomendado) Crie e ative um ambiente virtual:
    ```bash
    python -m venv venv
    # Windows
    .\venv\Scripts\activate
    # macOS/Linux
    source venv/bin/activate
    ```
3.  Instale as dependências:
    ```bash
    pip install -r requeriments.txt
    ```
4.  Execute o servidor:
    ```bash
    python app.py
    ```
    O servidor estará rodando, geralmente no endereço `http://127.0.0.1:5000`. Certifique-se de que o firewall não está bloqueando a porta.

### 3. Frontend (Controle Facial)

1.  Com a API rodando, navegue até a pasta `MediaPipe`.
2.  Devido às políticas de segurança dos navegadores (CORS), não é recomendado abrir o `holistic.html` diretamente. A melhor forma é servir os arquivos através de um servidor local simples. Se você tem Python instalado, pode fazer isso facilmente:
    ```bash
    # No terminal, dentro da pasta MediaPipe
    python -m http.server
    ```
3.  Abra o seu navegador e acesse `http://localhost:8000/holistic.html`.
4.  Permita o acesso à câmera. Agora você deve ver a sua imagem com os marcos faciais, e os gestos devem enviar comandos para a API.

### 4. Dashboard de Telemetria

1.  Com a API rodando, navegue até a pasta `Dashboard`.
2.  Assim como o frontend de controle, sirva os arquivos com um servidor local:
    ```bash
    # No terminal, dentro da pasta Dashboard
    python -m http.server
    ```
3.  Abra o seu navegador e acesse `http://localhost:8000/dashboard.html` ou `http://localhost:8000/graficos.html` para visualizar os dados.
