#include "HuskDevice.h"
#include "Sensor.h"
#include "SensorFactory.h"
#include "patch.h"
void HuskDevice::LigarLed()
{
	if (tipo != NODE_MCU)
	{
		if (LOGICA_INV_LED == true)
		{
			digitalWrite(LED_PIN, LOW);
		}
		else
		{
			digitalWrite(LED_PIN, HIGH);
		}
	}
	
}

void HuskDevice::DesligarLed()
{
	if (tipo != NODE_MCU)
	{
		if (LOGICA_INV_LED == true)
		{
			digitalWrite(LED_PIN, HIGH);
		}
		else
		{
			digitalWrite(LED_PIN, LOW);
		}
	}
	
}

void HuskDevice::LigarSonoff()
{
	if (tipo != NODE_MCU)
	{
		digitalWrite(OUTPUT_PIN, HIGH);
		SONOFF_LIGADO = 1;
	}
	
}

void HuskDevice::DesligarSonoff()
{
	if (tipo != NODE_MCU)
	{
		digitalWrite(OUTPUT_PIN, LOW);
		SONOFF_LIGADO = 0;
	}
	
}

void HuskDevice::InscreverTodosTopicos()
{
	int total = topicos.size();
	for (int i = 0; i < total; i++)
	{
		MQTT.subscribe(topicos.at(i).c_str());
	}
	MQTT.subscribe(this->ID_CLIENTE.c_str());
}

void HuskDevice::AdicionarTopico(std::string topico)
{
	int total = topicos.size();
	if (total <= 5)
	{

		//Primeiro verifica se o t�pico j� est� no vetor
		
		for (int i = 0; i < total; i++)
		{		
			if (topicos.at(i) == topico)
			{
				return;
			}
		}
		
		topicos.push_back(topico);
		MQTT.subscribe(topico.c_str());
	}
}

void HuskDevice::ImprimirTopicos() const
{
	/*for (Topico *aux = raizTopicos; aux != NULL; aux = aux->proximo)
	{
		Serial.printf("Topico: %s\n", aux->nome);
	}
	Serial.printf("-------------------\n");*/
}

void HuskDevice::RemoverTopico(std::string topico)
{
	int total = topicos.size();
	for (int i = 0; i < total; i++)
	{
		if (topicos.at(i) == topico)
		{
			topicos.erase(topicos.begin() + i);
			break;
		}
	}

}

void HuskDevice::EnviarMensagemLigado()
{
	std::string topico;
	char mensagem[2];

	itoa(SONOFF_LIGADO, mensagem, 10);

	topico.append(this->ID_CLIENTE);
	topico.append("/ligado");

	MQTT.publish(topico.c_str(), mensagem);
}


void HuskDevice::EnviarMensagemStatus()
{
	std::string topico;
	std::string mensagem;
	mensagem += this->SONOFF_STATUS;

	topico.append(this->ID_CLIENTE);
	topico.append("/status");

	MQTT.publish(topico.c_str(), mensagem.c_str());
}


void HuskDevice::ReconnectMQTT()
{
	
	if (MQTT.connect(this->ID_CLIENTE.c_str(), this->MQTT_USER.c_str(), this->MQTT_PASSWORD.c_str()))
	{
		Serial.printf("conectado mqtt");
		InscreverTodosTopicos();
		EnviarMensagemStatus();
		EnviarMensagemLigado();
	}
}

void HuskDevice::ReconnectWiFi() {
	while (!WiFi.isConnected()) {

	}
}

void HuskDevice::mqtt_callback(char* topic, byte* payload, unsigned int length)
{
	std::string comando;
	std::string chave;

	bool vezChave = false;
	for (int i = 0; i < length; i++)
	{
		char c = (char)payload[i];
		vezChave ? chave += c : comando += c;
		if (c == '\n')
		{
			vezChave = true;
		}
	}
	//Serial.printf("Comando: %s\nChave: %s\n", comando.c_str(), chave.c_str());

	if (comando == "tp")
	{
		chave == "1" ? LigarSonoff() : DesligarSonoff();
	}
	else if (comando == "sub")
	{
		std::vector<std::string> topicos = patch::split(chave, '\r');

		for (std::vector<std::string>::iterator it = topicos.begin(); it != topicos.end(); ++it)
		{
			AdicionarTopico(*it);
		}

	}
	else if (comando == "unsub")
	{
		RemoverTopico(chave);
	}
	else if (comando == "add_sensor")
	{
		std::vector<std::string> sensorParams = patch::split(chave, '\r'); //sensor = [0] e gpio = [1];

		std::unique_ptr<Sensor> novoSensor = SensorFactory::CriarSensor(sensorParams.at(0), std::atoi(sensorParams.at(1).c_str()));

		AdicionarSensor(std::move(novoSensor));
	}
	else if (comando == "rem_sensor")
	{
		RemoverSensor(std::atoi(chave.c_str()));
	}
	else if (comando == "sts")
	{
		SONOFF_STATUS = chave[0];
	}
	else
	{
		for (int y = 0; y < 5; y++) //Indica��o que deu algo de errado
		{
			LigarLed();
			delay(500);
			DesligarLed();
			delay(500);
		}
	}

	Serial.flush();
}


void HuskDevice::CriarID()
{
	String idstr = WiFi.macAddress();
	this->ID_CLIENTE = std::string(idstr.c_str());
}

HuskDevice::HuskDevice(TipoUpload stipo)
{
	switch (stipo)
	{
		case SONOFF_BASIC: //basic      
			OUTPUT_PIN = 12;
			LED_PIN = 13;
			BTN_PIN = 0;
			LOGICA_INV_LED = true;
			break;
		case SONOFF_POW: //pow
			OUTPUT_PIN = 12;
			LED_PIN = 15;
			BTN_PIN = 0;
			LOGICA_INV_LED = false;
			break;
		case NODE_MCU: //node_mcu
			LOGICA_INV_LED = false;
			break;
		default:
		{
			Serial.printf("Tipo inv�lido\n");
			return;
		}
	}
	tipo = stipo;
	Iniciar();
}

void HuskDevice::Iniciar()
{
	if (tipo != NODE_MCU)
	{
		pinMode(OUTPUT_PIN, OUTPUT);
		pinMode(LED_PIN, OUTPUT);
		pinMode(BTN_PIN, INPUT);
	}
	
	CriarID();
	SONOFF_STATUS = '0';
	MQTT = PubSubClient(espClient);
}

void HuskDevice::VerificarBtn()
{
	int btn_estado_atual = digitalRead(BTN_PIN);

	if (btn_estado_atual == 0 && btn_estado_atual != BTN_ESTADO)
	{
		if (SONOFF_LIGADO == 0)
		{
			LigarSonoff();
		}
		else
		{
			DesligarSonoff();
		}
		if (MQTT.connected())
		{
			EnviarMensagemLigado();
		}

	}
	BTN_ESTADO = btn_estado_atual;
}


void HuskDevice::Conectar(const std::string ssid, const std::string senha, const std::string servidor, int porta, const std::string usuariomqtt, const std::string senhamqtt)
{
	WiFi.begin(ssid.c_str(), senha.c_str());

	static unsigned long ultimo = millis();
	bool ligar = true;
	int intervaloLed = 150;
	while(!WiFi.isConnected())
	{
		if (tipo != NODE_MCU) //Se n�o � node_mcu
		{
			VerificarBtn();
			if ((millis() - ultimo) > intervaloLed)
			{

				ultimo = millis();
				if (ligar == true)
				{
					LigarLed();
					ligar = false;
				}
			}
			else
			{
				DesligarLed();
				ligar = true;
			}
			
		}
		delay(50); //Sem o delay o sonoff crasha
	}
	DesligarLed();

	this->MQTT_USER.assign(usuariomqtt);
	this->MQTT_PASSWORD.assign(senhamqtt);

	// DesligarLed();
	MQTT.setServer(servidor.c_str(), porta); //Endere�o de ip e porta do broker MQTT
	MQTT.setCallback(std::bind(&HuskDevice::mqtt_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void HuskDevice::Loop()
{
	if (tipo != NODE_MCU)
	{
		VerificarBtn();
	}

	if (!MQTT.connected()) {
		ReconnectMQTT();
	}
	else
	{
		for (std::vector<std::unique_ptr<Sensor>>::iterator itSensor = this->sensores.begin(); itSensor != this->sensores.end(); itSensor++)
		{
			Sensor* p = itSensor->get();
			if ((millis() - p->ultimoIntervalo) > p->intervalo)
			{
				p->ultimoIntervalo = millis();

				std::vector<MensagemMqtt> mensagens = p->executar();
				std::string topicoBase(this->ID_CLIENTE);

				for (std::vector<MensagemMqtt>::iterator itMensagem = mensagens.begin(); itMensagem != mensagens.end(); ++itMensagem)
				{
					std::string topico = topicoBase + "/" + itMensagem->topico;
					MQTT.publish(topico.c_str(), itMensagem->payload.c_str());
				}
			}
		}
		MQTT.loop();
	}
}

void HuskDevice::AdicionarSensor(std::unique_ptr<Sensor> s)
{
	sensores.push_back(std::move(s));
}

void HuskDevice::RemoverSensor(int gpio)
{
	int tamanho = sensores.size();
	for (int i = 0; i < tamanho; i++)
	{
		std::unique_ptr<Sensor> &sensorUnique = sensores.at(i);
		Sensor *sensor = sensorUnique.get();

		bool remover = sensor->getGPIO() == gpio;
		if (remover)
		{
			sensorUnique.release();
			sensores.erase(sensores.begin() + i);
			break;
		}
	}
}

int HuskDevice::GetBtn() const { return BTN_PIN; }
int HuskDevice::GetLed() const { return LED_PIN; }
PubSubClient HuskDevice::GetMQTT() const { return MQTT; }
int HuskDevice::GetOutput() const { return OUTPUT_PIN; }
char HuskDevice::GetStatus() const { return SONOFF_STATUS; }
std::string HuskDevice::GetID() const { return ID_CLIENTE; }
