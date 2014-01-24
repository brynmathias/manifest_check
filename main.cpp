#include "iostream"
#include "curl/curl.h"
#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_print.hpp"
#include <thread>
#include <vector>
#include <unistd.h>
#include <chrono>
using namespace rapidxml;
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}



std::string get_manifest_file( std::string manifest_url, CURL *curl )
{
    std::string buffer;

    CURLcode res;
        curl = curl_easy_init();
        curl_easy_setopt( curl, CURLOPT_URL, manifest_url.c_str() );
        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteCallback );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, &buffer );
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

    if(res == CURLE_OK)
        return buffer;
    else{
        std::cout << "failed curl";
        exit(0);
    }
}


long long get_time_stamp( std::string url, CURL *curl )
{
    std::string manifest = get_manifest_file(url, curl);    
    xml_document<> doc;    
    xml_node<> *root_node;
    char * cstr = new char[manifest.size() +1];
    // std::cout << manifest << std::endl;
    strcpy(cstr, manifest.c_str());
    doc.parse<0> ( cstr );
    // print(std::cout, doc, 0);
    root_node = doc.first_node("SmoothStreamingMedia");
    xml_node<> * video_node = root_node->first_node("StreamIndex");
    if(strcmp(video_node->first_attribute("Type")->value(), "video") != 0 && video_node->next_sibling() != NULL )
        video_node = video_node->next_sibling();
    xml_node<> *segment_node = video_node->first_node("c");
    return atoll(segment_node->first_attribute("t")->value());
}


void run_manifest_check(std::string url)
{
    int max_time_difference = 20020000;
    long long current_segment_time = 0;
    long long previous_segment_time = 0;
    CURL *curl;
    for(size_t i = 0; i < 500; ++i)
    {
        previous_segment_time = current_segment_time;
        current_segment_time = get_time_stamp(url,curl);
        if(current_segment_time - previous_segment_time > max_time_difference)
        {
            std::cout << "time difference is larger than expected at: " << current_segment_time - previous_segment_time << "\n URL: " << url <<std::endl;
        }
        std::chrono::milliseconds dura( 1500 );
        std::this_thread::sleep_for( dura );
    }

    
    
    
    
}




int main (int argc, char const *argv[])
{
    int num_threads = 8;
    std::vector<std::string> urls = {"http://hout.livec3.c9.id.cam7.eng.velocix.com/smj1/smooth/Manifest","http://hout.livec3.c9.id.cam7.eng.velocix.com/smj2/smooth/Manifest","http://hout.livec3.c9.id.cam7.eng.velocix.com/smj3/smooth/Manifest","http://hout.livec3.c9.id.cam7.eng.velocix.com/smj4/smooth/Manifest","http://hout.livec3.c9.id.cam7.eng.velocix.com/smj5/smooth/Manifest","http://hout.livec3.c9.id.cam7.eng.velocix.com/smj6/smooth/Manifest","http://hout.livec3.c9.id.cam7.eng.velocix.com/smj7/smooth/Manifest","http://hout.livec3.c9.id.cam7.eng.velocix.com/smj8/smooth/Manifest"};
    std::vector<std::thread> t;    
    // std::cout << "video time stamp = " << get_time_stamp( "http://hout.livec3.c9.id.cam7.eng.velocix.com/smj1/smooth/Manifest") << std::endl;
    for (int i = 0; i < num_threads; ++i) {
         t.push_back(std::thread(run_manifest_check, urls[i]));
     }
     
     for (auto &th: t) {
         th.join();
     }
    // print(std::cout, doc, 0);   // 0 means default printing flags
    // print(std::cout, root_node, 0);
    return 0;
}