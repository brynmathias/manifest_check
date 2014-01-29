#include <iostream>
#include <fstream>
#include "curl/curl.h"
#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_print.hpp"
#include <thread>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <chrono>
#include <stdio.h>
#include <string.h>
using namespace rapidxml;
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}




long long get_time_stamp( std::string manifest )
{
    xml_document<> doc;
    xml_node<> *root_node;
    char * cstr = new char[manifest.size() +1];
    strcpy(cstr, manifest.c_str());
    doc.parse<0> ( cstr );
    root_node = doc.first_node("SmoothStreamingMedia");
    xml_node<> * video_node = root_node->first_node("StreamIndex");
    if(strcmp(video_node->first_attribute("Type")->value(), "video") != 0 && video_node->next_sibling() != NULL )
        video_node = video_node->next_sibling();
    xml_node<> *segment_node = video_node->first_node("c");
    return atoll(segment_node->first_attribute("t")->value());
}


void run_manifest_check(int channel)
{

    char url[100];
    sprintf(url,"http://hout.livec3.c9.id.cam7.eng.velocix.com/smj%d/smooth/Manifest",
            channel);
    std::string buffer;
    CURL *curl;
    int count = 0;
    CURLcode res;
    curl = curl_easy_init();
    curl_easy_setopt( curl, CURLOPT_URL, url );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteCallback );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, &buffer );
    std::stringstream localBuffer;
    int max_time_difference = 20020000;
    long long current_segment_time = 0;
    long long previous_segment_time = 0;
    int max_time = 0;
    bool clear_counter = false;
    int update_counter = 0;
    for(size_t i = 0; i < 500; ++i)
    {
        res = curl_easy_perform(curl);
        if(current_segment_time != previous_segment_time){
            clear_counter = true;
            previous_segment_time = current_segment_time;
        }
        else
        {
            update_counter += 1;
        }
        current_segment_time = get_time_stamp(buffer);
        if(current_segment_time - previous_segment_time > max_time_difference)
        {
            count++;
            if(count > 1){
                localBuffer << "time difference is larger than expected at: " << current_segment_time - previous_segment_time 
                    << " count is "<< count << "\n URL: " << url << "\n took " << update_counter << " manifest refreshes to update." << std::endl;
                max_time += current_segment_time - previous_segment_time;
            }
        }
        if(clear_counter == true)
        {
            update_counter = 0;
            clear_counter = false;

        }
        std::chrono::milliseconds dura( 500 );
        std::this_thread::sleep_for( dura );
        buffer.clear();
    }

    char fname[50];
    localBuffer << "!!!!!!!!!\n average time over 2 seconds is " << (max_time)/(count-1) << " for n = " << count << std::endl;
    sprintf(fname,"./stream_%d.log", channel);
    std::ofstream file;
    file.open(fname, std::ios::out);
    file << localBuffer.str();

    file.close();
    curl_easy_cleanup(curl);

}




int main (int argc, char const *argv[])
{
    int num_threads = 199;
    std::vector<std::thread> t;
    // std::cout << "video time stamp = " << get_time_stamp( "http://hout.livec3.c9.id.cam7.eng.velocix.com/smj1/smooth/Manifest") << std::endl;
    for (int i = 0; i < num_threads; ++i) {
         t.push_back(std::thread(run_manifest_check, i+1));
     }

     for (auto &th : t) {
         th.join();
     }
    // print(std::cout, doc, 0);   // 0 means default printing flags
    // print(std::cout, root_node, 0);
    return 0;
}
