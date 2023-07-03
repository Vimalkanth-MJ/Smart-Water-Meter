# Functions Explained:

📌 `sendMessage(String Outgoing, byte Destination)`: This function is responsible for sending a LoRa message. It takes the outgoing message (`Outgoing`) and the destination address (`Destination`) as parameters. It begins the LoRa packet, adds the destination address, sender address, payload length, and the actual payload. Finally, it ends the packet and sends it.

📌 `onReceive(int packetSize)`: This function is called when a LoRa message is received. It takes the packet size as a parameter. Inside the function, it reads the packet header bytes, including the recipient and sender addresses, and the incoming message length. Then it reads the incoming message and stores it in the `Incoming` variable. If the message is intended for the local device (based on the recipient address), it calls the `Processing_incoming_data()` function to process the incoming data.

📌 `Processing_incoming_data()`: This function processes the incoming data received over LoRa. It extracts different values from the incoming message using string manipulation functions (`indexOf()` and `substring()`). It retrieves the reading ID, total liters consumed, available credits, and cost per liter. It updates the relevant variables and displays the received data if debugging is enabled.

📌 `getReadings()`: This function is responsible for calculating the flow rate, total liters consumed, available credits, and sending LoRa data. It first checks if there are available credits. If credits are available, it calculates the flow rate and updates the flow and credits variables. It turns on the solenoid valve for water flow, displays the data on the integrated display, and sends the data over LoRa. If there are no available credits, it resets the values, turns off the solenoid valve, displays a "No Data" message, and requests updated data from the master device over LoRa.

📌 `sendLoRaData()`: This function constructs the LoRa message with the updated data and sends it to the master node. It creates a message string by concatenating the device ID, total liters consumed, available credits, and cost per liter. Then it calls the `sendMessage()` function to send the message to the master node.

📌 `increase()`: This function is used as an interrupt handler for the flow sensor. It increases the pulse count and pulse1 count, which are used in calculating the flow rate and total liters consumed.

📌 `DisplayData()`: This function displays the flow-related data on the integrated screen. It uses the Adafruit SSD1306 library to initialize the display, set the text size and color, and position the cursor to print the data.

📌 `DisplayNoData()`: This function displays a "No Data" message on the integrated screen when there are no available credits. It clears the display, sets the text size and color, and prints the message.

📌 `setup()`: This function is called once when the program starts. It initializes the serial communication, LoRa module, sensor pin, relay pin, and interrupt for the flow sensor. It also initializes the OLED display, sets up the display parameters, and retrieves initial data from the master device over LoRa.

📌 `loop()`: This function is called repeatedly after the `setup()` function. It checks for any incoming LoRa packets using `onReceive()`, updates the flow-related data using `getReadings()`, and continues the program execution.

These functions work together to establish LoRa communication, receive and process incoming data, calculate flow-related information, display data on the integrated screen, and send updated data to the master node.


## Code WorkFlow

1. Start
2. Setup: Initialize the program and set up the necessary components, such as LoRa, pins, and the OLED display.
3. LoRa Initialized: Verify if LoRa initialization is successful. If not, display an error message.
4. Initialize Components: Set up the sensor pin as an input and the relay pin as an output. Attach an interrupt to the sensor pin to count pulses.
5. Request Updated Data from Master: Send a request to the master device over LoRa to get the latest data.
6. Receive LoRa Packet: Check if there is an incoming LoRa packet.
7. Process Incoming Data: If a packet is received, extract the relevant data from it and store it for further processing.
8. Calculate Flow Data: Calculate the flow rate, total volume consumed, available credits, and credits consumed based on the flow and cost per liter.
9. Update Display: Clear the OLED display and update it with the latest data, including the total volume, flow rate, and available credits.
10. Send LoRa Data: Send the updated data (total volume, available credits, and cost per liter) to the master device over LoRa.
11. Check Available Credits: Check if there are available credits for water consumption.
12. Display No Data: If there are no available credits, display a message indicating that credits need to be recharged.
13. Request Updated Data from Master: Send another request to the master device over LoRa to get the latest data.
14. Receive LoRa Packet: Check if there is an incoming LoRa packet.
15. End: Repeat the program flow from step 5 until the program is terminated.

This sequence of steps represents the main functionality of the program, which involves communication with the master device, processing and updating flow-related data, displaying information on the OLED display, and ensuring the availability of credits for water consumption.
