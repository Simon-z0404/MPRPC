#include <iostream>
#include "test1.pb.h"

using namespace fixbug;
using namespace std;

int main() {

    getFriendListRespone rsp;
    ResultCode* rc = rsp.mutable_res();
    rc->set_errcode(0);
    
    Usr* usr1 = rsp.add_friend_list();
    usr1->set_name("Tom");
    usr1->set_age(22);
    usr1->set_sex(Usr::men);

    Usr* usr2 = rsp.add_friend_list();
    usr2->set_name("Marry");
    usr2->set_age(24);
    usr2->set_sex(Usr::women);

    cout << rsp.friend_list_size() << endl;

    return 0;
}