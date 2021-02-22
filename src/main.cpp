#include <iostream>
#include <string>
#include <tgbot/tgbot.h>
#include <filesystem>
#include <chrono>
#include <vector>
#include <thread>
#include <map>

namespace fs = std::filesystem;

struct path_leaf_string
{
    std::string operator()(const std::filesystem::directory_entry& entry) const
    {
        //return entry.path().leaf().string();
        return entry.path().string();
    }
};

void read_directory(const std::string& name, std::vector<std::string>& v)
{
    std::filesystem::path p(name);
    std::filesystem::directory_iterator start(p);
    std::filesystem::directory_iterator end;
    std::transform(start, end, std::back_inserter(v), path_leaf_string());
}

void send_petrovich(TgBot::Bot &bot, std::int64_t chat_id, std::string s, bool disable_notification)
{
    const std::string photoMimeType = "image/jpeg";
    for(int cnt = 5; ; cnt++)
    {
        try
        {
            std::vector<std::string> photos;
            read_directory("../photos", photos);
            cnt = cnt%photos.size();
            std::cout << photos[cnt] << '\n';
            auto msg = bot.getApi().sendPhoto(chat_id, TgBot::InputFile::fromFile(photos[cnt], photoMimeType));
            bot.getApi().pinChatMessage(chat_id, msg->messageId, false);
            bot.getApi().deleteMessage(chat_id, msg->messageId+1);
            sleep(60*60);
        } catch (std::exception e) {std::cerr << "error " << chat_id << ' ' << s << '\n';}
    }
}

int main()
{
    TgBot::Bot bot(bot_api_key);
    /*bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hi!");
    });*/
    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        //printf("User wrote %s\n", message->text.c_str());
        //std::cout << "User " << message->from->username << " in chat type " << message->chat->title << " wrote " << message->text.c_str() << '\n';
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        //bot.getApi().sendMessage(message->chat->id, "Your message is: " + message->text);
    });
    std::map<std::int64_t, bool> opened;
    std::vector<std::thread> ths;
    bot.getEvents().onCommand("photo", [&bot, &opened, &ths](TgBot::Message::Ptr message)
    {
        std::cout << "received photo requst\n";
        if(!opened[message->chat->id])
        {
            opened[message->chat->id] = true;
            ths.push_back(std::thread{send_petrovich,std::ref(bot), message->chat->id, message->chat->username, true});
            std::cout << "out";
        }
    });
    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        std::cout << "AAAAAA\n";
        printf("error: %s\n", e.what());
    }
    for(auto &i:ths)
        i.join();
    return 0;
}
