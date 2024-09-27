// midi_test.cpp

#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>
#include <chrono>
#include <vector>

// Function to get the MIDI endpoint by name
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

// Global variables for the output port and destination
MIDIPortRef outputPort = 0;
MIDIEndpointRef outputEndpoint = 0;

// Callback function for MIDI input
void midiInputCallback(const MIDIPacketList* packetList, void* readProcRefCon, void* srcConnRefCon) {
    auto receiveTime = std::chrono::high_resolution_clock::now();

    for (unsigned int i = 0; i < packetList->numPackets; ++i) {
        const MIDIPacket& packet = packetList->packet[i];

        // Log the received message
        std::cout << "Received message: ";
        for (unsigned int j = 0; j < packet.length; ++j) {
            printf("%02X ", packet.data[j]);
        }
        std::cout << std::endl;

        // Send the message back out
        Byte buffer[1024];
        MIDIPacketList* outPacketList = (MIDIPacketList*)buffer;
        MIDIPacket* outPacket = MIDIPacketListInit(outPacketList);
        outPacket = MIDIPacketListAdd(outPacketList, sizeof(buffer), outPacket, 0, packet.length, packet.data);

        if (outPacket == NULL) {
            std::cerr << "Error creating MIDIPacketList" << std::endl;
            return;
        }

        OSStatus result = MIDISend(outputPort, outputEndpoint, outPacketList);
        if (result != noErr) {
            std::cerr << "Error sending MIDI message: " << result << std::endl;
            return;
        }

        auto sendTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(sendTime - receiveTime).count();
        double elapsedTimeMs = elapsedTime / 1000.0;
        std::cout << "Elapsed time between receive and send: " << elapsedTimeMs << " ms" << std::endl;
    }
}

int main() {
    std::cout << "Test: Receive from Virtual Device -> Send to Virtual Device" << std::endl;

    // Create a MIDI client
    MIDIClientRef midiClient;
    OSStatus result = MIDIClientCreate(CFSTR("CoreMIDI Test Client"), NULL, NULL, &midiClient);
    if (result != noErr) {
        std::cerr << "Error creating MIDI client: " << result << std::endl;
        return 1;
    }

    // Create an input port
    MIDIPortRef inputPort;
    result = MIDIInputPortCreate(midiClient, CFSTR("Input Port"), midiInputCallback, NULL, &inputPort);
    if (result != noErr) {
        std::cerr << "Error creating MIDI input port: " << result << std::endl;
        return 1;
    }

    // Create an output port
    result = MIDIOutputPortCreate(midiClient, CFSTR("Output Port"), &outputPort);
    if (result != noErr) {
        std::cerr << "Error creating MIDI output port: " << result << std::endl;
        return 1;
    }

    // List available MIDI input and output ports
    listMIDIPorts(true);  // List inputs
    listMIDIPorts(false); // List outputs

    // Specify your MIDI device name
    std::string deviceName = "APC Key 25"; // Change this to your device's name

    // Get the MIDI source (input)
    MIDIEndpointRef inputEndpoint = getMIDIEndpointByName(deviceName, true);
    if (inputEndpoint == 0) {
        std::cerr << "MIDI input port not found." << std::endl;
        return 1;
    }

    // Get the MIDI destination (output)
    outputEndpoint = getMIDIEndpointByName(deviceName, false);
    if (outputEndpoint == 0) {
        std::cerr << "MIDI output port not found." << std::endl;
        return 1;
    }

    // Connect the input port to the source
    result = MIDIPortConnectSource(inputPort, inputEndpoint, NULL);
    if (result != noErr) {
        std::cerr << "Error connecting MIDI input port: " << result << std::endl;
        return 1;
    }

    std::cout << "Listening for MIDI input... Press Ctrl+C to exit." << std::endl;

    // Keep the program running
    CFRunLoopRun();

    // Cleanup (this part may not be reached if the program is terminated with Ctrl+C)
    MIDIPortDisconnectSource(inputPort, inputEndpoint);
    MIDIPortDispose(inputPort);
    MIDIPortDispose(outputPort);
    MIDIClientDispose(midiClient);

    return 0;
}
