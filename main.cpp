#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <cstdio>
#include <limits> 

using namespace std;

class LogStore {
private:
    string fname;
    fstream f;
    mutex m;
    map<string, streampos> mp; // Sorted map for alphabetical range queries

    // Data Integrity: Sum of ASCII values
    unsigned int getCheck(const string& k, const string& v) {
        unsigned int sum = 0;
        for(char c : k) sum += (unsigned char)c;
        for(char c : v) sum += (unsigned char)c;
        return sum;
    }

    // Binary format: [Status][Checksum][KLen][Key][VLen][Value]
    void write(const string& k, const string& v, bool del) {
        char s = del ? 0x00 : 0x01;
        size_t kl = k.length();
        size_t vl = v.length();
        unsigned int check = getCheck(k, v);

        f.write(&s, sizeof(s));
        f.write(reinterpret_cast<const char*>(&check), sizeof(check));
        f.write(reinterpret_cast<const char*>(&kl), sizeof(kl));
        f.write(k.c_str(), kl);
        f.write(reinterpret_cast<const char*>(&vl), sizeof(vl));
        f.write(v.c_str(), vl);
        f.flush();
    }

    void load() {
        f.seekg(0, ios::beg);
        while(f.peek() != EOF) {
            streampos p = f.tellg();
            char s;
            unsigned int savedCheck;
            size_t kl, vl;

            if (!f.read(&s, sizeof(s))) break;
            f.read(reinterpret_cast<char*>(&savedCheck), sizeof(savedCheck));
            f.read(reinterpret_cast<char*>(&kl), sizeof(kl));
            
            string k(kl, ' ');
            f.read(&k[0], kl);
            
            f.read(reinterpret_cast<char*>(&vl), sizeof(vl));
            string v(vl, ' ');
            f.read(&v[0], vl);

            // Verify data hasn't been corrupted
            if(getCheck(k, v) != savedCheck) {
                cout << "[!] Corruption at " << p << ". Stopping load." << endl;
                break;
            }

            if(s == 0x01) mp[k] = p;
            else mp.erase(k);
        }
        f.clear(); 
    }

public:
    LogStore(string name) {
        fname = name;
        f.open(fname, ios::in | ios::out | ios::binary | ios::app);
        if(!f.is_open()) {
            ofstream c(fname, ios::binary);
            c.close();
            f.open(fname, ios::in | ios::out | ios::binary | ios::app);
        }
        load();
    }

    ~LogStore() {
        if(f.is_open()) f.close();
    }

    void set(string k, string v) {
        lock_guard<mutex> lock(m);
        f.seekp(0, ios::end);
        streampos p = f.tellp();
        write(k, v, false);
        mp[k] = p;
    }

    string get(string k) {
        lock_guard<mutex> lock(m);
        if(mp.find(k) == mp.end()) return "NOT_FOUND";

        f.seekg(mp[k]);
        char s;
        unsigned int c;
        size_t kl, vl;
        f.read(&s, sizeof(s));
        f.read(reinterpret_cast<char*>(&c), sizeof(c));
        f.read(reinterpret_cast<char*>(&kl), sizeof(kl));
        f.seekg(kl, ios::cur); 
        f.read(reinterpret_cast<char*>(&vl), sizeof(vl));
        
        string v(vl, ' ');
        f.read(&v[0], vl);
        return v;
    }

    vector<pair<string, string>> get_range(string start, string end) {
        lock_guard<mutex> lock(m);
        vector<pair<string, string>> results;
        auto it_start = mp.lower_bound(start);
        auto it_end = mp.upper_bound(end);

        for(auto it = it_start; it != it_end; ++it) {
            results.push_back({it->first, get(it->first)});
        }
        return results;
    }

    void remove(string k) {
        lock_guard<mutex> lock(m);
        if(mp.find(k) == mp.end()) return;
        f.seekp(0, ios::end);
        write(k, "", true); 
        mp.erase(k);
    }

    void compact() {
        lock_guard<mutex> lock(m);
        string tname = "temp_" + fname;
        ofstream tf(tname, ios::binary);
        map<string, streampos> nmp;

        for(auto const& pair : mp) {
            string val = get(pair.first);
            streampos np = tf.tellp();
            
            char s = 0x01;
            size_t kl = pair.first.length(), vl = val.length();
            unsigned int c = getCheck(pair.first, val);
            
            tf.write(&s, sizeof(s));
            tf.write(reinterpret_cast<const char*>(&c), sizeof(c));
            tf.write(reinterpret_cast<const char*>(&kl), sizeof(kl));
            tf.write(pair.first.c_str(), kl);
            tf.write(reinterpret_cast<const char*>(&vl), sizeof(vl));
            tf.write(val.c_str(), vl);
            
            nmp[pair.first] = np;
        }

        tf.close();
        f.close();
        std::remove(fname.c_str());
        std::rename(tname.c_str(), fname.c_str());
        f.open(fname, ios::in | ios::out | ios::binary | ios::app);
        mp = nmp;
    }
};

int main() {
    LogStore db("engine.db");
    int choice;
    string k, v, s_key, e_key;

    while(true) {
        cout << "\n===============================" << endl;
        cout << "   LogStore Engine" << endl;
        cout << "===============================" << endl;
        cout << "1. SET (Add/Update)" << endl;
        cout << "2. GET (Find Key)" << endl;
        cout << "3. RANGE SCAN (A to Z)" << endl;
        cout << "4. DELETE (Remove Key)" << endl;
        cout << "5. COMPACT (Log Cleaning)" << endl;
        cout << "6. EXIT" << endl;
        cout << "-------------------------------" << endl;
        cout << "Selection: ";

        if(!(cin >> choice)) {
            cout << "\n[!] Invalid input. Numbers only please." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if(choice == 6) break;

        switch(choice) {
            case 1:
                cout << "Key: "; cin >> k;
                cout << "Value: "; cin >> v;
                db.set(k, v);
                cout << "[+] Success." << endl;
                break;
            case 2:
                cout << "Search Key: "; cin >> k;
                cout << "Result: " << db.get(k) << endl;
                break;
            case 3:
                cout << "Start Key: "; cin >> s_key;
                cout << "End Key: "; cin >> e_key;
                {
                    auto res = db.get_range(s_key, e_key);
                    cout << "\n--- Scanned Entries ---" << endl;
                    if(res.empty()) cout << "No data found in range." << endl;
                    for (auto p : res) cout << " [" << p.first << "] -> " << p.second << endl;
                }
                break;
            case 4:
                cout << "Key to Delete: "; cin >> k;
                db.remove(k);
                cout << "[-] Key removed from index." << endl;
                break;
            case 5:
                cout << "[*] Starting compaction..." << endl;
                db.compact();
                cout << "[+] Optimized. Stale data removed." << endl;
                break;
            default:
                cout << "[!] Please choose 1-6." << endl;
        }
    }
    cout << "\nClosing engine. Happy coding, Raj!" << endl;
    return 0;
}