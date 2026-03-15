#include <iostream>
#include <string>
#include <curl/curl.h> 

// A standard callback function that tells cURL how to handle the response data from the node. 
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb); 
    return size * nmemb; 
}

std::string fire_rpc_request(const std::string& rpc_url, const std::string& json_payload) {
    CURL* curl;
    CURLcode res; 
    std::string readBuffer; 
    
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, rpc_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());

        // Capture the response from the blockchain node
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer); 

        // Fire the request
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "Network Error: " << curl_easy_strerror(res) << std:: endl;
        } 
        // Clean up memory
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return readBuffer;
}