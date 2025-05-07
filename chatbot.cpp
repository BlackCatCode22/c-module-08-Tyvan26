#include <iostream>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <json/json.h>  // For JSON handling

// Global variables to track conversation history and iterations
int chatIterationCount = 0;
std::vector<std::pair<std::string, std::string>> conversationHistory; // Stores user and chatbot exchanges
std::chrono::duration<double> totalResponseTime(0); // To calculate average response time

// Function to handle API responses and store conversation history
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to send the user query to the OpenAI API and get the response
std::string sendApiRequest(const std::string& userInput) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set up the request (example for OpenAI API)
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/completions");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Authorization: Bearer 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"); // Replace with your API key
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Build JSON payload for the API request
        Json::Value root;
        root["model"] = "text-davinci-003";
        root["prompt"] = userInput;
        root["max_tokens"] = 100;
        root["temperature"] = 0.7;

        Json::StreamWriterBuilder writer;
        std::string jsonPayload = Json::writeString(writer, root);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());

        // Perform the request and handle potential errors
        auto start = std::chrono::high_resolution_clock::now();
        res = curl_easy_perform(curl);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        totalResponseTime += elapsed; // Add response time to total

        if (res != CURLE_OK) {
            std::cerr << "CURL failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();
    return readBuffer;
}

// Recursive function to retry API request on failure
std::string recursiveApiRequest(const std::string& userInput, int retries = 3) {
    std::string response = sendApiRequest(userInput);

    if (response.empty() && retries > 0) {
        std::cout << "Error occurred, retrying..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Short delay before retrying
        return recursiveApiRequest(userInput, retries - 1);
    }

    return response;
}

// Function to handle user input with error checking
std::string getUserInput() {
    std::string userInput;
    while (true) {
        std::cout << "You: ";
        std::getline(std::cin, userInput);

        // Check for empty input
        if (userInput.empty()) {
            std::cout << "Input cannot be empty. Please enter a valid input." << std::endl;
            continue;
        }

        // Check for character limit (let's assume the limit is 200 characters)
        if (userInput.length() > 200) {
            std::cout << "Input is too long. Please limit your input to 200 characters." << std::endl;
            continue;
        }

        break;
    }
    return userInput;
}

int main() {
    std::cout << "Chatbot Initialized. Type 'exit' to quit the conversation." << std::endl;

    // Main chat loop
    while (true) {
        std::string userInput = getUserInput();

        // Handle exit condition
        if (userInput == "exit") {
            break;
        }

        // Send the user query to the API and get the response
        auto start = std::chrono::high_resolution_clock::now();
        std::string apiResponse = recursiveApiRequest(userInput);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        totalResponseTime += elapsed;

        // Parse the API response (simplified)
        Json::CharReaderBuilder reader;
        Json::Value responseJson;
        std::istringstream sstream(apiResponse);
        std::string errs;
        Json::parseFromStream(reader, sstream, &responseJson, &errs);

        // Display chatbot's response
        std::string chatbotResponse = responseJson["choices"][0]["text"].asString();
        std::cout << "Chatbot: " << chatbotResponse << std::endl;

        // Track the conversation history
        chatIterationCount++;
        conversationHistory.push_back({"You: " + userInput, "Chatbot: " + chatbotResponse});

        // Display conversation history after each iteration
        std::cout << "\nConversation History (Iteration " << chatIterationCount << "):" << std::endl;
        for (const auto& entry : conversationHistory) {
            std::cout << entry.first << "\n" << entry.second << "\n";
        }

        // Display average response time
        double averageResponseTime = totalResponseTime.count() / chatIterationCount;
        std::cout << "Average response time: " << averageResponseTime << " seconds." << std::endl;
    }

    return 0;
}
