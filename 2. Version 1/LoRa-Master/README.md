
# Code WorkFlow

1. Initialize the serial communication and LoRa module.
2. Connect to the Blynk server using the specified authentication token and Wi-Fi credentials.
3. Set up a timer to periodically send data to the Blynk app.
4. Enter the `loop()` function.
5. Check for incoming LoRa packets using `LoRa.parsePacket()`.
6. If a packet is received, go to the `onReceive(int packetSize)` function.
7. Read the packet header to get the recipient, sender, and message length.
8. Read the payload data and store it in the `Incoming` variable.
9. Check the length of the incoming message for data integrity.
10. If the message is for the current device, process it accordingly.
11. If the message starts with "Hi", prepare a response message based on the sender's address and send it using LoRa.
12. If the message is a data update, parse the values for `volume1`, `credits1`, and `cost1`, and update the variables.
13. Send the updated data to the Blynk app using the respective virtual pins.
14. Exit the `onReceive()` function and continue with the `loop()` function.
15. Run the Blynk library using `Blynk.run()` to maintain the Blynk connection.
16. Run the Blynk timer using `timer.run()` to trigger the periodic data update to the Blynk app.
17. Repeat the loop and continue listening for LoRa packets and updating the Blynk app.

This flow ensures that the ESP32 receives LoRa packets, processes the received data, updates the Blynk app, and maintains the communication with both LoRa and Blynk services.

## Functions Explained:

📌 `BLYNK_CONNECTED()`: This is a Blynk library callback function that is called when the ESP32 connects to the Blynk server. In this code, it is used to synchronize the virtual pins V1, V3, V4, and V5 with their corresponding values on the Blynk server.

📌 `BLYNK_WRITE(V1)`: This is another Blynk callback function that is called whenever the value of virtual pin V1 is updated on the Blynk app. In this code, it updates the `credits1` variable with the new value received from the app and then calls `sendDataToClient()` and `computeRemainingVolume()` functions.

📌 `BLYNK_WRITE(V3)`: This function is similar to the previous one but it is triggered when the value of virtual pin V3 is updated on the Blynk app. It updates the `volume1` variable with the new value received from the app and calls `sendDataToClient()` function.

📌 `BLYNK_WRITE(V4)`: This function is triggered when the value of virtual pin V4 is updated on the Blynk app. It updates the `remainingVolume1` variable with the new value received from the app.

📌 `BLYNK_WRITE(V5)`: This function is triggered when the value of virtual pin V5 is updated on the Blynk app. It updates the `cost1` variable with the new value received from the app and calls `computeRemainingVolume()` and `sendDataToClient()` functions.

📌 `computeRemainingVolume()`: This function calculates the remaining volume based on the `credits1` and `cost1` variables and updates the `remainingVolume1` variable. It also sends the updated `remainingVolume1` value to the Blynk app using the virtual pin V4.

📌 `sendDataToClient()`: This function constructs a message using the `volume1`, `credits1`, and `cost1` variables and sends it to the specified destination device (Destination_ESP32_Slave_1) using LoRa communication. The message format is "Slave01/volume1&credits1#cost1".

📌 `sendDataToBlynk()`: This function sends the current values of `credits1`, `volume1`, `remainingVolume1`, and `cost1` to the Blynk app using their respective virtual pins V1, V3, V4, and V5.

📌 `ResetToDefaultData()`: This function sets the initial/default values for `cost1`, `volume1`, `credits1`, `cost2`, `volume2`, and `credits2`. It then calls `sendDataToBlynk()` to update the Blynk app with the default values.

📌 `sendMessage(String Outgoing, byte Destination)`: This function is responsible for sending a message (Outgoing) to the specified destination device (Destination) using LoRa communication. It starts by beginning a LoRa packet, writes the destination address, sender address, payload length, and payload data, and then ends the packet and sends it.

📌 `onReceive(int packetSize)`: This function is called when a LoRa packet is received. It reads the packet header bytes to determine the recipient, sender, and length of the incoming message. Then it reads the payload data and stores it in the `Incoming` variable. It checks the length of the incoming message and compares it to the received length to ensure data integrity. If the message is for the current device, it processes the message accordingly. If the message starts with "Hi", it prepares a response message based on the sender's address. If the message is a data update, it parses the values for `volume1`, `credits1`, and `cost1` and updates the variables accordingly. Finally, it calls `sendDataToBlynk()` to update the Blynk app with the received values.

📌 `setup()`: This function is called once during the initialization of the ESP32. It initializes the serial communication, sets up LoRa pins, and initializes the LoRa module. It also initializes the Blynk connection with the specified authentication token and Wi-Fi credentials. Additionally, it sets up a timer to periodically call `sendDataToBlynk()` every 1 second.

📌 `loop()`: This function is called repeatedly in a loop. It checks for incoming LoRa packets using `LoRa.parsePacket()`, processes the received packets using `onReceive()`, runs the Blynk library using `Blynk.run()`, and also runs the Blynk timer using `timer.run()`. This function ensures continuous communication with LoRa and Blynk services.
