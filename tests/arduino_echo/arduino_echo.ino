namespace
{
constexpr unsigned long kSerialBaudrate = 115200;
} // namespace

void setup()
{
    Serial.begin(kSerialBaudrate);
}

void loop()
{
    bool echoed = false;

    while (Serial.available() > 0)
    {
        const int incoming = Serial.read();
        if (incoming < 0)
        {
            break;
        }

        Serial.write(static_cast<uint8_t>(incoming));
        echoed = true;
    }

    if (echoed)
    {
        Serial.flush();
    }
}
