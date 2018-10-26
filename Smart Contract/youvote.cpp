#include <eosiolib/eosio.hpp>

using namespace eosio;

class youvote : public contract {
  public:
      youvote(eosio::name s):contract(s), _polls(s, s), _votes(s, s)
      {}

      // public methods exposed via the ABI
      // on pollsTable

      [[eosio::action]]
      void version()
      {
          print("YouVote version  0.01"); 

      };
      
      [[eosio::action]]
      void addpoll(eosio::name s, std::string pollName)
      {
          //require_auth(s);

          print("Add poll ", pollName); 
              
          // update the table to include a new poll
          _polls.emplace(get_self(), [&](auto& p)
                                      {
                                        p.key = _polls.available_primary_key();
                                        p.pollId = _polls.available_primary_key();
                                        p.pollName = pollName;
                                        p.pollStatus = 0;
                                        p.option = "";
                                        p.count = 0;
                                      });
      };


      [[eosio::action]]
      void rmpoll(eosio::name s, std::string pollName)
      {
          //require_auth(s);

          print("Remove poll ", pollName); 
              
          std::vector<uint64_t> keysForDeletion;
          // find items which are for the named poll
          for(auto& item : _polls)
          {
              if (item.pollName == pollName)
              {
                  keysForDeletion.push_back(item.key);   
              }
          }
          
          // now delete each item for that poll
          for (uint64_t key : keysForDeletion)
          {
              print("remove from _polls ", key);
              auto itr = _polls.find(key);
              if (itr != _polls.end())
              {
                _polls.erase(itr);
              }
          }


          // add remove votes ... don't need it the actions are permanently stored on the block chain

          std::vector<uint64_t> keysForDeletionFromVotes;
          // find items which are for the named poll
          for(auto& item : _votes)
          {
              if (item.pollName == pollName)
              {
                  keysForDeletionFromVotes.push_back(item.key);   
              }
          }
          
          // now delete each item for that poll
          for (uint64_t key : keysForDeletionFromVotes)
          {
              print("remove from _votes ", key);
              auto itr = _votes.find(key);
              if (itr != _votes.end())
              {
                _votes.erase(itr);
              }
          }


      };

      [[eosio::action]]
      void status(std::string pollName)
      {
          print("Change poll status ", pollName);

          std::vector<uint64_t> keysForModify;
          // find items which are for the named poll
          for(auto& item : _polls)
          {
              if (item.pollName == pollName)
              {
                  keysForModify.push_back(item.key);   
              }
          }
          
          // now get each item and modify the status
          for (uint64_t key : keysForModify)
          {

            print("modify _polls status", key);
            auto itr = _polls.find(key);
            if (itr != _polls.end())
            {
              _polls.modify(itr, get_self(), [&](auto& p)
                                              {
                                                p.pollStatus = p.pollStatus + 1;
                                              });
            }
          }
      };

      [[eosio::action]]
      void statusreset(std::string pollName)
      {
          print("Reset poll status ", pollName); 
              
          std::vector<uint64_t> keysForModify;
          // find all poll items
          for(auto& item : _polls)
          {
              if (item.pollName == pollName)
              {
                  keysForModify.push_back(item.key);   
              }
          }
          
          // update the status in each poll item
          for (uint64_t key : keysForModify)
          {
              print("modify _polls status", key);
              auto itr = _polls.find(key);
              if (itr != _polls.end())
              {
                _polls.modify(itr, get_self(), [&](auto& p)
                                                {
                                                  p.pollStatus = 0;
                                                });
              }
          }
      };


      [[eosio::action]]
      void addpollopt(std::string pollName, std::string option)
      {
          print("Add poll option ", pollName, "option ", option); 

          // find the pollId, from _polls, use this to update the _polls with a new option
          for(auto& item : _polls)
          {
              if (item.pollName == pollName)
              {
                    // can only add if the poll is not started or finished
                    if(item.pollStatus == 0)
                    {
                        _polls.emplace(get_self(), [&](auto& p)
                                          {
                                            p.key = _polls.available_primary_key();
                                            p.pollId = item.pollId;
                                            p.pollName = item.pollName;
                                            p.pollStatus = 0;
                                            p.option = option;
                                            p.count = 0;
                                          });
                    }
                    else
                    {
                        print("Can not add poll option ", pollName, "option ", option, " Poll has started or is finished.");
                    }

                    break; // so you only add it once
              }
          }
      };

      [[eosio::action]]
      void rmpollopt(std::string pollName, std::string option)
      {
          print("Remove poll option ", pollName, "option ", option); 
              
          std::vector<uint64_t> keysForDeletion;
          // find and remove the named poll
          for(auto& item : _polls)
          {
              if (item.pollName == pollName)
              {
                  keysForDeletion.push_back(item.key);   
              }
          }
          
          
          for (uint64_t key : keysForDeletion)
          {
              print("remove from _polls ", key);
              auto itr = _polls.find(key);
              if (itr != _polls.end())
              {
                  if (itr->option == option)
                  {
                      _polls.erase(itr);
                  }
              }
          }
      };


      [[eosio::abi]]
      void vote(std::string pollName, std::string option, std::string accountName)
      {
          print("vote for ", option, " in poll ", pollName, " by ", accountName); 

          // is the poll open
          for(auto& item : _polls)
          {
              if (item.pollName == pollName)
              {
                  if (item.pollStatus != 1)
                  {
                      print("Poll ",pollName,  " is not open");
                      return;
                  }

                  break; // only need to check status once
              }
          }

          // has account name already voted?  
          for(auto& vote : _votes)
          {
              if (vote.pollName == pollName && vote.account == accountName)
              {
                  print(accountName, " has already voted in poll ", pollName);
                  //eosio_assert(true, "Already Voted");
                  return;
              }
          }

          uint64_t pollId =99999; // get the pollId for the _votes table

          // find the poll and the option and increment the count
          for(auto& item : _polls)
          {
              if (item.pollName == pollName && item.option == option)
              {
                  pollId = item.pollId; // for recording vote in this poll

                  _polls.modify(item, get_self(), [&](auto& p)
                                                {
                                                    p.count = p.count + 1;
                                                });
              }
          }

          // record that accountName has voted
          _votes.emplace(get_self(), [&](auto& pv)
                                      {
                                        pv.key = _votes.available_primary_key();
                                        pv.pollId = pollId;
                                        pv.pollName = pollName;
                                        pv.account = accountName;
                                      });        
      };

  private:    

    // 创建多索引表来保存数据
      struct [[eosio::table]] poll 
      {
        uint64_t      key; // 主键
        uint64_t      pollId; // 次要键, 不是唯一的, 由于option,该表会为每一次轮询提供重复行
        std::string   pollName; // poll的名字
        uint8_t      pollStatus =0; // staus where 0 = closed, 1 = open, 2 = finished
        std::string  option; // 你可以投票的条目
        uint32_t    count =0; // 每一次的票数 -- 它被拉取下来以区分表.

        uint64_t primary_key() const { return key; }
        uint64_t by_pollId() const {return pollId; }
      };
      typedef eosio::multi_index<N(poll), poll, indexed_by<N(pollId), const_mem_fun<poll, uint64_t, &poll::by_pollId>>> pollstable;

      struct [[eosio::table]] pollvotes 
      {
         uint64_t     key; 
         uint64_t     pollId;
         std::string  pollName; // 投票的名字
         std::string  account; //该账户投过的, 用这个来确保没有人投票> 1

         uint64_t primary_key() const { return key; }
         uint64_t by_pollId() const {return pollId; }
      };
      typedef eosio::multi_index<name(pollvotes), pollvotes, indexed_by<name(pollId), const_mem_fun<pollvotes, uint64_t, &pollvotes::by_pollId>>> votes;

      // local instances of the multi indexes
      pollstable _polls;
      votes _votes;
};

EOSIO_DISPATCH( youvote, (version)(addpoll)(rmpoll)(status)(statusreset)(addpollopt)(rmpollopt)(vote))
