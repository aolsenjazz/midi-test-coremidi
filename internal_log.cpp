// midi_test.cpp

#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>
#include <chrono>

// Callback function for the virtual MIDI destination
void virtualDestinationCallback(const MIDIPacketList* packetList, void* readProcRefCon, void* srcConnRefCon) {
    for (unsigned int i = 0; i < packetList->numPackets; ++i) {
        const MIDIPacket& packet = packetList->packet[i];

        // Record the receive time
        auto receiveTime = std::chrono::high_resolution_clock::now();

        // Convert time to milliseconds since epoch
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(receiveTime.time_since_epoch()).count();

        // Log the received message and time
        std::cout << "Received " << ms << " ms: ";
        for (unsigned int j = 0; j < packet.length; ++j) {
            printf("%02X ", packet.data[j]);
        }
        std::cout << std::endl;
    }
}

// Struct to hold callback data for the input port
struct InputCallbackData {
    MIDIPortRef outputPort;
    MIDIEndpointRef destinationEndpoint; // virtualDestination
};

// Callback function for MIDI input from APC Key 25
void inputCallback(const MIDIPacketList* packetList, void* readProcRefCon, void* srcConnRefCon) {
    // Convert time to milliseconds since epoch
    auto receiveTime = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(receiveTime.time_since_epoch()).count();

    // Log the received message and time
    std::cout << "From APC: " << ms << " ms: ";

    InputCallbackData* callbackData = static_cast<InputCallbackData*>(readProcRefCon);

    MIDIPortRef outputPort = callbackData->outputPort;
    MIDIEndpointRef destinationEndpoint = callbackData->destinationEndpoint;

    for (unsigned int i = 0; i < packetList->numPackets; ++i) {
        const MIDIPacket& packet = packetList->packet[i];

        // Send the packet to the virtual destination
        Byte buffer[1024];
        MIDIPacketList* outPacketList = (MIDIPacketList*)buffer;
        MIDIPacket* outPacket = MIDIPacketListInit(outPacketList);
        outPacket = MIDIPacketListAdd(outPacketList, sizeof(buffer), outPacket, 0, packet.length, packet.data);

        if (outPacket == NULL) {
            std::cerr << "Error creating MIDIPacketList" << std::endl;
            return;
        }

        OSStatus result = MIDISend(outputPort, destinationEndpoint, outPacketList);
        // Convert time to milliseconds since epoch
        // Record the receive time
        auto receiveTime2 = std::chrono::high_resolution_clock::now();
        auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(receiveTime2.time_since_epoch()).count();

        // Log the received message and time
        std::cout << "Sent: " << ms2 << " ms: ";
        if (result != noErr) {
            std::cerr << "Error sending MIDI message: " << result << std::endl;
            return;
        }
    }
}

// Function to get MIDI endpoints by name
MIDIEndpointRef getMIDIEndpointByName(const std::string& name, bool isSource) {
    ItemCount count = isSource ? MIDIGetNumberOfSources() : MIDIGetNumberOfDestinations();
    for (ItemCount i = 0; i < count; ++i) {
        MIDIEndpointRef endpoint = isSource ? MIDIGetSource(i) : MIDIGetDestination(i);
        if (endpoint != 0) {
            CFStringRef endpointName = NULL;
            MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &endpointName);
            if (endpointName != NULL) {
                char cstr[256];
                Boolean result = CFStringGetCString(endpointName, cstr, sizeof(cstr), kCFStringEncodingUTF8);
                CFRelease(endpointName);
                if (result && name == cstr) {
                    return endpoint;
                }
            }
        }
    }
    return 0;
}

// Function to list available MIDI endpoints
void listMIDIPorts(bool isSource) {
    ItemCount count = isSource ? MIDIGetNumberOfSources() : MIDIGetNumberOfDestinations();
    std::cout << "Available MIDI " << (isSource ? "Input" : "Output") << " Ports:" << std::endl;
    for (ItemCount i = 0; i < count; ++i) {
        MIDIEndpointRef endpoint = isSource ? MIDIGetSource(i) : MIDIGetDestination(i);
        if (endpoint != 0) {
            CFStringRef endpointName = NULL;
            MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &endpointName);
            if (endpointName != NULL) {
                char cstr[256];
                Boolean result = CFStringGetCString(endpointName, cstr, sizeof(cstr), kCFStringEncodingUTF8);
                CFRelease(endpointName);
                if (result) {
                    std::cout << i << ": " << cstr << std::endl;
                }
            }
        }
    }
}

int main() {
    std::cout << "Creating virtual MIDI destination..." << std::endl;

    // Create a MIDI client
    MIDIClientRef midiClient = 0;
    OSStatus result = MIDIClientCreate(CFSTR("CoreMIDI Test Client"), NULL, NULL, &midiClient);
    if (result != noErr) {
        std::cerr << "Error creating MIDI client: " << result << std::endl;
        return 1;
    }

    // Create a virtual destination
    MIDIEndpointRef virtualDestination = 0;
    result = MIDIDestinationCreate(midiClient, CFSTR("Test Virtual Destination"), virtualDestinationCallback, NULL, &virtualDestination);
    if (result != noErr) {
        std::cerr << "Error creating virtual destination: " << result << std::endl;
        return 1;
    }

    std::cout << "Virtual MIDI destination 'Test Virtual Destination' created." << std::endl;

    // Create an output port to send messages to the virtual destination
    MIDIPortRef outputPort = 0;
    result = MIDIOutputPortCreate(midiClient, CFSTR("Output Port"), &outputPort);
    if (result != noErr) {
        std::cerr << "Error creating output port: " << result << std::endl;
        return 1;
    }

    // Create an input port to receive messages from APC Key 25
    MIDIPortRef inputPort = 0;

    // Prepare callback data for input port
    InputCallbackData inputCallbackData;
    inputCallbackData.outputPort = outputPort;
    inputCallbackData.destinationEndpoint = virtualDestination;

    result = MIDIInputPortCreate(midiClient, CFSTR("Input Port"), inputCallback, &inputCallbackData, &inputPort);
    if (result != noErr) {
        std::cerr << "Error creating input port: " << result << std::endl;
        return 1;
    }

    // Get the MIDI source (input) endpoint for APC Key 25
    std::string deviceName = "APC Key 25"; // Change this to your device's name

    MIDIEndpointRef inputEndpoint = getMIDIEndpointByName(deviceName, true);
    if (inputEndpoint == 0) {
        std::cerr << "MIDI input endpoint not found." << std::endl;
        return 1;
    }

    // Connect the input port to the source endpoint
    result = MIDIPortConnectSource(inputPort, inputEndpoint, NULL);
    if (result != noErr) {
        std::cerr << "Error connecting MIDI input port to source: " << result << std::endl;
        return 1;
    }

    std::cout << "Listening for MIDI input from '" << deviceName << "'. Press Ctrl+C to exit." << std::endl;

    // Keep the program running
    CFRunLoopRun();

    // Cleanup (this part may not be reached if the program is terminated with Ctrl+C)
    MIDIPortDisconnectSource(inputPort, inputEndpoint);
    MIDIEndpointDispose(virtualDestination);
    MIDIPortDispose(inputPort);
    MIDIPortDispose(outputPort);
    MIDIClientDispose(midiClient);

    return 0;
}
