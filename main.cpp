#include<iostream>
#include<fstream>
#include<string>
#include<map>
#include<mutex>
#include<limits>
#include<cstdio>
using namespace std;

class LogStore {
private:
    string filename;
    map<string, long long> index; // Map any index with the file postion(long long)
    mutex mtx; // To ensure only one thread write file at a time

    // Calculate checksum
    int calculateChecksum(const string& key, const string& value) {
        int sum = 0;
        for(char c:key) sum += c;
        for(char c:value) sum += c;
        return sum;
    }

    // This function will read data from file in O(logn) time
    string readValueAt(long long pos) {
        // opening file
        ifstream file(filename, ios::binary);

        // jump file pointer to specified byte position
        file.seekg(pos);

        int savedCheckSum, kLen, vLen;
        // reading header
        if(!(file >> savedCheckSum >> kLen >> vLen)) return "CORRUPTED";
        
        // skiping space between header and data
        file.get();

        // reading key
        string key(kLen, ' ');
        file.read(&key[0], kLen);

        // if record if deleted
        if(vLen == -1) return "NOT FOUND";

        // reading value
        string value(vLen, ' ');
        file.read(&value[0], vLen);

        // matching checksum
        if(calculateChecksum(key, value) == savedCheckSum) return value;
        return "Data Altered"; // file is corrupted
    }

public:
    LogStore(string filename) {
        this->filename = filename;
        loadIndex();
    }

    void loadIndex() {
        // opening file in read format
        ifstream file(filename, ios::binary);
        if(!file.is_open()) return; // if file not exist -> return

        long long currentPos = 0;
        
        while(file.peek() != EOF) {
            // Get the byte position of the current record
            currentPos = file.tellg();
            
            int cs, kLen, vLen;
            if (!(file >> cs >> kLen >> vLen)) break; // reading headers
            
            file.get(); // skiping space separator
            // reading key
            string key(kLen, ' ');
            file.read(&key[0], kLen);

            // updating index map
            if(vLen == -1) {
                index.erase(key);
            } else {
                index[key] = currentPos;
                file.seekg(vLen, ios::cur);
            }
            
            // Skip any newlines or carriage returns to get to the next record
            while(file.peek() == '\n' || file.peek() == '\r') file.get();
        }
        cout << "Index loaded. Keys in RAM: " << index.size() << endl;
    }

    // Write data or delete it
    void set(string key, string value, bool isDelete = false) {
        // Lock the database so nobody else can write right now
        lock_guard<mutex> lock(mtx);
        
        // opening file in "append" mode and moving write pointer to end of file
        ofstream file(filename, ios::binary | ios::app);
        file.seekp(0, ios::end);
        long long pos = file.tellp(); // finding byte position

        int kLen = key.length();
        // if deleting vLen is -1
        int vLen = isDelete ? -1 : value.length();
        // calculating checksum
        int cs = isDelete ? calculateChecksum(key, "") : calculateChecksum(key, value);

        // Write the data in our format
        file << cs << " " << kLen << " " << vLen << " " << key;
        // writing data if we are not deleting
        if(!isDelete) file << value;
        // newline to separate records
        file << "\n";

        // remove key if deleting else update value of key in map
        if(isDelete) index.erase(key);
        else index[key] = pos;
    }

    // Retrieval
    string get(string key) {
        lock_guard<mutex> lock(mtx); // Lock for safety
        if(index.find(key) == index.end()) return "NOT FOUND"; // key does not exist
        return readValueAt(index[key]); // read value
    }

    // Delete logic
    void remove(string key) {
        // removing key if it exist
        if(index.count(key)) {
            set(key, "", true); 
        }
    }

    // Cleaning the file - compact
    void compact() {
        // stop all other actions during compaction
        lock_guard<mutex> lock(mtx);
        // creating a new file and opening it in write mode
        string temp = "temp_db.txt";
        ofstream out(temp, ios::binary);
        // new index map
        map<string, long long> newIdx;

        for(auto &[key, pos]:index) {
            string val = readValueAt(pos);
            // skiping corrupted data
            if(val == "Data altered") continue;

            // new byte position
            long long newPos = out.tellp();
            int kLen = key.length();
            int vLen = val.length();
            int cs = calculateChecksum(key, val);

            // write data to new file
            out << cs << " " << kLen << " " << vLen << " " << key << val << "\n";
            // updating index map
            newIdx[key] = newPos;
        }
        // closing file
        out.close();
        // deleting old file and renaming new file to the database name
        std::remove(filename.c_str());
        std::rename(temp.c_str(), filename.c_str());
        index = newIdx; // updating map
        cout << "Database compacted" << endl;
    }
};


bool isInputOk(string s) {
    return (s.length() == 1 && s[0] >= '1' && s[0] <= '5');
}
int main() {
    LogStore db("database.txt");
    string input, key, value;

    while(true) {
        cout << "\n--- LOGSTORE ENGINE ---\n1. SET  2. GET  3. DELETE  4. COMPACT  5. EXIT\nSelection: ";
        cin >> input;

        if(!isInputOk(input)) {
            cout << "Invalid input! Please enter value between 1 and 5" << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        int choice = input[0] - '0';
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // clearing input buffer

        switch(choice) {
            case 1: {
                cout << "Key: ";
                getline(cin, key);
                cout << "Value: ";
                getline(cin, value);
                db.set(key, value);
                cout << "Record added to database" << endl;
                break;
            }
            case 2: {
                cout << "Key: ";
                getline(cin, key);
                cout << "Result: " << db.get(key) << endl;
                break;
            }
            case 3: {
                cout << "Key to Delete: ";
                getline(cin, key);
                db.remove(key);
                cout << "Key deleted" << endl;
                break;
            }
            case 4: {
                db.compact();
                break;
            }
            case 5: {
                cout << "Thank You!" << endl;
                return 0;
            }
        }
    }
    return 0;
}
