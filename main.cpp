#include<iostream>
#include<fstream>
#include<string>
#include<map>
#include<vector>
#include<cstdio>
#include<mutex>
#include<limits> 
using namespace std;

class LogStore {
private:
    string filename;
    fstream file;
    map<string, long long> index;
    mutex mtx;

    unsigned int calculateChecksum(string k, string v) {
        unsigned int sum = 0;
        for (char c : k) {
            sum += (unsigned char)c;
        }
        for (char c : v) {
            sum += (unsigned char)c;
        }
        return sum;
    }

    string getValueAtOffset(long long pos) {
        file.seekg(pos);
        char status;
        unsigned int checksum;
        int kLen, vLen;

        file.read(&status, 1);
        file.read((char*)&checksum, sizeof(checksum));
        file.read((char*)&kLen, sizeof(kLen));
        file.seekg(kLen, ios::cur); 
        file.read((char*)&vLen, sizeof(vLen));
        
        string value(vLen, ' ');
        file.read(&value[0], vLen);
        return value;
    }

    void writeEntry(string k, string v, bool isDeleted) {
        char status = isDeleted ? 0 : 1;
        int kLen = k.length();
        int vLen = v.length();
        unsigned int checksum = calculateChecksum(k, v);

        file.write(&status, 1);
        file.write((char*)&checksum, sizeof(checksum));
        file.write((char*)&kLen, sizeof(kLen));
        file.write(k.c_str(), kLen);
        file.write((char*)&vLen, sizeof(vLen));
        file.write(v.c_str(), vLen);
        file.flush();
    }

    void loadIndex() {
        cout << "Loading database index, please wait..." << endl;
        file.seekg(0, ios::beg);
        int count = 0;
        while (file.peek() != EOF) {
            long long pos = file.tellg();
            char status;
            unsigned int savedChecksum;
            int kLen, vLen;

            if (!file.read(&status, 1)) break;
            file.read((char*)&savedChecksum, sizeof(savedChecksum));
            file.read((char*)&kLen, sizeof(kLen));
            
            string key(kLen, ' ');
            file.read(&key[0], kLen);
            
            file.read((char*)&vLen, sizeof(vLen));
            string value(vLen, ' ');
            file.read(&value[0], vLen);

            if (calculateChecksum(key, value) != savedChecksum) break;

            if (status == 1) {
                index[key] = pos;
            } else {
                index.erase(key);
            }
            count++;
        }
        file.clear(); 
        cout << "Loaded " << count << " log entries successfully." << endl;
    }

public:
    LogStore(string name) {
        filename = name;
        file.open(filename, ios::in | ios::out | ios::binary | ios::app);
        if (!file.is_open()) {
            ofstream create(filename, ios::binary);
            create.close();
            file.open(filename, ios::in | ios::out | ios::binary | ios::app);
        }
        loadIndex();
    }

    ~LogStore() {
        if (file.is_open()) {
            file.close();
        }
    }

    void set(string k, string v) {
        lock_guard<mutex> lock(mtx);
        file.seekp(0, ios::end);
        long long pos = file.tellp();
        writeEntry(k, v, false);
        index[k] = pos;
    }

    string get(string k) {
        lock_guard<mutex> lock(mtx);
        if (index.find(k) == index.end()) {
            return "NOT_FOUND";
        }
        return getValueAtOffset(index[k]);
    }

    vector<pair<string, string>> getRange(string start, string end) {
        lock_guard<mutex> lock(mtx);
        vector<pair<string, string>> results;
        auto itStart = index.lower_bound(start);
        auto itEnd = index.upper_bound(end);

        for (auto it = itStart; it != itEnd; ++it) {
            results.push_back({it->first, getValueAtOffset(it->second)});
        }
        return results;
    }

    void remove(string k) {
        lock_guard<mutex> lock(mtx);
        if (index.find(k) == index.end()) {
            return;
        }
        file.seekp(0, ios::end);
        writeEntry(k, "", true); 
        index.erase(k);
    }

    void compact() {
        lock_guard<mutex> lock(mtx);
        string tempName = "temp_" + filename;
        ofstream tempFile(tempName, ios::binary);
        map<string, long long> newIndex;

        for (auto const& entry : index) {
            string val = getValueAtOffset(entry.second);
            long long newPos = tempFile.tellp();
            
            char status = 1;
            int kLen = entry.first.length();
            int vLen = val.length();
            unsigned int checksum = calculateChecksum(entry.first, val);
            
            tempFile.write(&status, 1);
            tempFile.write((char*)&checksum, sizeof(checksum));
            tempFile.write((char*)&kLen, sizeof(kLen));
            tempFile.write(entry.first.c_str(), kLen);
            tempFile.write((char*)&vLen, sizeof(vLen));
            tempFile.write(val.c_str(), vLen);
            
            newIndex[entry.first] = newPos;
        }

        tempFile.close();
        file.close();
        std::remove(filename.c_str());
        std::rename(tempName.c_str(), filename.c_str());
        file.open(filename, ios::in | ios::out | ios::binary | ios::app);
        index = newIndex;
    }
};

int main() {
    LogStore db("engine.db");
    int choice;
    string k, v, s, e;

    while (true) {
        cout << "\n===============================" << endl;
        cout << "   LogStore Engine Interface" << endl;
        cout << "===============================" << endl;
        cout << "1. SET (Add/Update)" << endl;
        cout << "2. GET (Find Key)" << endl;
        cout << "3. RANGE SEARCH (Lexical order)" << endl;
        cout << "4. DELETE (Remove Key)" << endl;
        cout << "5. COMPACT (Log Cleaning)" << endl;
        cout << "6. EXIT" << endl;
        cout << "-------------------------------" << endl;
        cout << "Selection: ";
        
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Numbers only please." << endl;
            continue;
        }
        
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1:
                cout << "Key: "; 
                getline(cin, k);
                cout << "Value: "; 
                getline(cin, v);
                db.set(k, v);
                cout << "Success." << endl;
                break;

            case 2:
                cout << "Search Key: "; 
                getline(cin, k);
                cout << "Result: " << db.get(k) << endl;
                break;

            case 3:
                cout << "Start Key: "; 
                getline(cin, s);
                cout << "End Key: "; 
                getline(cin, e);
                {
                    auto res = db.getRange(s, e);
                    cout << "\n--- Scanned Entries ---" << endl;
                    if (res.empty()) {
                        cout << "No data found in range." << endl;
                    } else {
                        for (auto const& p : res) {
                            cout << " [" << p.first << "] -> " << p.second << endl;
                        }
                    }
                }
                break;

            case 4:
                cout << "Key to Delete: "; 
                getline(cin, k);
                db.remove(k);
                cout << "Key removed from index." << endl;
                break;

            case 5:
                cout << "Starting compaction..." << endl;
                db.compact();
                cout << "Optimized. Stale data removed." << endl;
                break;

            case 6:
                cout << "Closing engine. Happy coding!" << endl;
                return 0;

            default:
                cout << "Invalid choice." << endl;
        }
    }
    return 0;
}
