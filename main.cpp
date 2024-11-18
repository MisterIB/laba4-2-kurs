#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <semaphore>
#include <condition_variable>
#include <barrier>
#include <atomic>
#include <vector>
#include <set>
#include <iomanip>
#include <unistd.h>
#include <random>
#include <chrono>

using namespace std;

void menu() {
    cout << "[1] - Задание 1" << endl;
    cout << "[2] - Задание 2" << endl;
    cout << "[3] - Задание 3" << endl;
    cout << "[0] - Выход" << endl;
    cout << "> ";
}

struct SemaphoreSlim {
public:
    void wait() {
        unique_lock<mutex> lock(mtx);
        while (countOfThreads == 0) {
            cond.wait(lock);
        }
        countOfThreads--;
    }

    void release() {
        lock_guard<mutex> lock(mtx);
        countOfThreads++;
        cond.notify_one();
    }

private:
    int32_t countOfThreads = 1;
    mutex mtx;
    condition_variable cond;
};

struct Monitor {
public:
    Monitor() : accessControl(false) {}

    void enter() {
        unique_lock<mutex> lock(mtx);
        while (accessControl) {
            cond.wait(lock);
        }
        accessControl = true;
    }


    void exit() {
        lock_guard<mutex> lock(mtx);
        accessControl = false;
        cond.notify_one();
    }

private:
    bool accessControl;
    mutex mtx;
    condition_variable cond;
};

struct Primitives {
public:
    mutex mtx;
    SemaphoreSlim sem;
    atomic_flag spinLock = ATOMIC_FLAG_INIT;
};

barrier bar {10};

void workWithMutexes(Primitives& primitives, char randomCharacter, int32_t numberThread, string& result) {
    primitives.mtx.lock();
    result += "Поток: " + to_string(numberThread)  + " Символ: " + randomCharacter + "\n";
    primitives.mtx.unlock();
}

void workWithSemaphore(counting_semaphore<1>& semaphore, char randomCharacter, int32_t numberThread, string& result) {
    semaphore.acquire();
    result += "Поток: " + to_string(numberThread)  + " Символ: " + randomCharacter + " | " + randomCharacter + "\n";
    semaphore.release(); 
}

void workWithSemaphoreSlim(Primitives& primitives, char randomCharacter, int32_t numberThread, string& result) {
    primitives.sem.wait();
    result += "Поток: " + to_string(numberThread)  + " Символ: " + randomCharacter + "\n";
    primitives.sem.release();
}

void workWithBarrier(Primitives& primitives, char randomCharacter, int32_t numberThread, string& result) {
    primitives.mtx.lock();
    result += "Поток: " + to_string(numberThread)  + " Символ: " + randomCharacter + " дошел до барьера\n";
    primitives.mtx.unlock();

    bar.arrive_and_wait();

    primitives.mtx.lock();
    result += "Поток: " + to_string(numberThread)  + " Символ: " + randomCharacter + " прошел барьер\n";
    primitives.mtx.unlock();

}

void workWithSpinLock(Primitives& primitives, char randomCharacter, int32_t numberThread, string& result) {
    while (primitives.spinLock.test_and_set());
    result += "Поток: " + to_string(numberThread)  + " Символ: " + randomCharacter + "\n";
    primitives.spinLock.clear();
}

void workWithSpinWait(Primitives& primitives, char randomCharacter, int32_t numberThread, string& result) {
    while (primitives.spinLock.test_and_set()) { this_thread::yield(); }
    result += "Поток: " + to_string(numberThread)  + " Символ: " + randomCharacter + "\n";
    primitives.spinLock.clear();
}

void workWithMonitor(Monitor& monitor, char randomCharacter, int32_t numberThread, string& result) {
    monitor.enter();
    result += "Поток: " + to_string(numberThread)  + " Символ: " + randomCharacter + "\n";
    monitor.exit();
}

void workWithoutSynch(char randomCharacter, int32_t numberThread, string& result) {
    result += "Поток: " + to_string(numberThread)  + " Символ: " + randomCharacter + "\n";
}

void generateThreads(int32_t inputData, int32_t amountOfThreads) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(33, 127);
    Primitives primitives;
    counting_semaphore<1> semaphore(1);
    Monitor monitor;
    vector<thread> threads;
    string result;
    for (int32_t i = 0; i < amountOfThreads; i++) {
        if (inputData == 1) threads.emplace_back(workWithMutexes, ref(primitives), distrib(gen), i, ref(result));
        else if (inputData == 2) threads.emplace_back(workWithSemaphore, ref(semaphore), distrib(gen), i, ref(result));
        else if (inputData == 3) threads.emplace_back(workWithSemaphoreSlim, ref(primitives), distrib(gen), i, ref(result));
        else if (inputData == 4) threads.emplace_back(workWithBarrier, ref(primitives), distrib(gen), i, ref(result));
        else if (inputData == 5) threads.emplace_back(workWithSpinLock, ref(primitives), distrib(gen), i, ref(result));
        else if (inputData == 6) threads.emplace_back(workWithSpinWait, ref(primitives), distrib(gen), i, ref(result));
        else if (inputData == 7) threads.emplace_back(workWithMonitor, ref(monitor), distrib(gen), i, ref(result));
        else if (inputData == 8) threads.emplace_back(workWithoutSynch, distrib(gen), i, ref(result));
    }
    for (thread& t: threads) {
        t.join();
    }
    cout << result;
}

void printNamePrimitives(int32_t numberPrimitive) {
    if (numberPrimitive == 1) cout << "Mutex" << endl;
    if (numberPrimitive == 2) cout << "Semaphore" << endl;
    if (numberPrimitive == 3) cout << "SemaphoreSlim" << endl; 
    if (numberPrimitive == 4) cout << "Barrier" << endl; 
    if (numberPrimitive == 5) cout << "SpinLock" << endl;
    if (numberPrimitive == 6) cout << "SpinWait" << endl;
    if (numberPrimitive == 7) cout << "Monitor" << endl;
    if (numberPrimitive == 8) cout << "Work without primitives" << endl;
}

void choosePrimitive() {
    cout << "введите количество потоков\n>";
    int32_t amountOfThreads;
    cin >> amountOfThreads;
    for (int32_t i = 1; i <= 8; i++) {
        printNamePrimitives(i);
        auto start = chrono::high_resolution_clock::now();
        generateThreads(i, amountOfThreads);
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> duration = end - start;
        cout << "Время работы: " << duration << "\n\n";
    }
}

struct Shop {
public:
    string humanSettlements;
    string street;
    string house;
    int32_t UID;
};

struct TimeCounter {
    clock_t startMultiThread;
    clock_t endMultiThread;
    clock_t startOneThread;
    clock_t endOneThread;
};

void printShops(const string& result) {
    cout << result << endl;
}

string createShopInformation(Shop shop) {
    return "UID: " + to_string(shop.UID) + " | Human settlements: " + shop.humanSettlements + " | street: " + shop.street + " | house: " + shop.house + "\n";
}

void searchShops(string street, const vector<Shop>& shops) {
    string result = "";
    int32_t counterShops = 0;
    set<string> humanSettlements;
    for (size_t i = 0; i < shops.size(); i++) {
        if (shops[i].street == street and humanSettlements.find(shops[i].humanSettlements) == humanSettlements.end()) {
            result += createShopInformation(shops[i]);
            counterShops++;
            humanSettlements.insert(shops[i].humanSettlements);
        }
    }
    if (counterShops >= 2) printShops(result);
}

void choosingStreet(set<string>& allStreets, vector<Shop>& shops, mutex& mtx) {
    if (allStreets.size() == 0) return;
    while (true) {
    if (allStreets.size() == 0) return;
    mtx.lock();
    string street = *allStreets.begin();
    allStreets.erase(street);
    mtx.unlock();
    searchShops(street, shops);
    }
}

void choosingStreetNoMultiThrd(set<string>& allStreets, vector<Shop>& shops) {
    while (true) {
    if (allStreets.size() == 0) break;
    string street = *allStreets.begin();
    allStreets.erase(street);
    searchShops(street, shops);
    }
}

set<string> searchAllStreets(const vector<Shop>& shops) {
    set<string> allStreets;
    for (size_t i = 0; i < shops.size(); i++) {
        allStreets.insert(shops[i].street);
    }
    return allStreets;
}

chrono::duration<double> creatingThreads(vector<Shop>& shops, int32_t amountOfParallelThreads) {
    set<string> allStreets = searchAllStreets(shops);
    mutex mtx;
    vector<thread> threads;
    auto startMulti = chrono::high_resolution_clock::now();
    for (int32_t i = 0; i < amountOfParallelThreads; i++) {
        threads.emplace_back(choosingStreet, ref(allStreets), ref(shops), ref(mtx));
    }
    for (thread& t: threads) {
        t.join();
    }
    auto endMulti = chrono::high_resolution_clock::now();
    chrono::duration<double> timeMulti = endMulti - startMulti;
    return timeMulti;
}

void searchWithoutMultithread(vector<Shop>& shops) {
    set<string> allStreets = searchAllStreets(shops);
    choosingStreetNoMultiThrd(allStreets, shops);
}

vector<Shop> creatingShops(int32_t sizeOfDataArray) {
    vector<Shop> shops;
    ifstream shopsFile("shops.data");
    for (int32_t i = 0; i < sizeOfDataArray; i++) {
        Shop shop;
        shopsFile >> shop.humanSettlements;
        shopsFile >> shop.street;
        shopsFile >> shop.house;
        shop.UID = i;
        shops.push_back(shop);
    }
    return shops;
}

void enterInputDataToSearchShops() {
    cout << "Введите размер массива данных: ";
    int32_t sizeOfDataArray;
    cin >> sizeOfDataArray;
    cout << "Введите количество параллельных потоков: ";
    int32_t amountOfParallelThreads;
    cin >> amountOfParallelThreads;
    vector<Shop> shops;
    shops = creatingShops(sizeOfDataArray);
    cout << "С многопоточностью" << endl;
    chrono::duration<double> timeMulti = creatingThreads(shops, amountOfParallelThreads);
    cout << "Без многопоточности" << endl;
    auto startSingle = chrono::high_resolution_clock::now();
    searchWithoutMultithread(shops);
    auto endSingle = chrono::high_resolution_clock::now();
    chrono::duration<double> timeSingle = endSingle - startSingle;
    cout << "Без многопоточности " << timeSingle << endl;
    cout << "C многопоточностью " << timeMulti << endl;
}

struct User {
public:
    User(string userType): userType(userType){}
    string userType;
    bool isReader() {
        if (userType == "READER") return true;
        if (userType == "WRITER") return false;
        throw runtime_error("Incorrect type of user");
    }

    bool isWriter() {
        if (userType == "READER") return false;
        if (userType == "WRITER") return true;
        throw runtime_error("Incorrect type of user");
    }

    void action(int32_t ID) {
    if (userType == "READER") {
        cout << "Reader " << ID << " reading" << endl;
    }
    else if (userType == "WRITER") {
        cout << "Writer " << ID << " writing" << endl;
        sleep(2);
    }
    else throw runtime_error("Incorrect type of user");
    }
};

struct CounterUsers {
    int32_t readerCount;
    int32_t writerCount;
    int32_t waitWriterCount;
    CounterUsers() {
        readerCount = 0;
        writerCount = 0;
        waitWriterCount = 0;
    }
};

struct Mtx {
    mutex lock_action;
    mutex lock_wait;
};

void waitWriter(CounterUsers& counterUsers) {
    while (counterUsers.readerCount > 0 or counterUsers.writerCount > 0);
    counterUsers.writerCount += 1;
}

void noWaitReader(CounterUsers& counterUsers) {
     while (counterUsers.writerCount > 0);
     counterUsers.readerCount += 1;
}

void solutionPriorityRead(User user, CounterUsers& counterUsers, int32_t ID, Mtx& mtx) {
    mtx.lock_wait.lock();
    if (user.isWriter()) waitWriter(counterUsers);
    if (user.isReader()) noWaitReader(counterUsers);
    mtx.lock_wait.unlock();
    mtx.lock_action.lock();
    user.action(ID);
    if (user.isWriter()) counterUsers.writerCount -= 1;
    if (user.isReader()) counterUsers.readerCount -= 1;
    mtx.lock_action.unlock();
}

void waitReader(CounterUsers& counterUsers) {
    while (counterUsers.writerCount > 0 or counterUsers.waitWriterCount > 0);
    counterUsers.readerCount += 1;
}

void noWaitWriter(CounterUsers& counterUsers) {
    counterUsers.waitWriterCount += 1;
    while (counterUsers.writerCount > 0);
    counterUsers.waitWriterCount -= 1;
    counterUsers.writerCount += 1;
}

void solutionPriorityWrite(User user, CounterUsers& counterUsers, int32_t ID, Mtx& mtx) {
    mtx.lock_wait.lock();
    if (user.isWriter()) noWaitWriter(counterUsers);
    if (user.isReader()) waitReader(counterUsers);
    mtx.lock_wait.unlock();
    mtx.lock_action.lock();
    user.action(ID);
    if (user.isWriter()) counterUsers.writerCount -= 1;
    if (user.isReader()) counterUsers.readerCount -= 1;
    mtx.lock_action.unlock();
}

void createThreads(int32_t priority) {
    CounterUsers counterUsers;
    Mtx mtx;
    random_device rd;
    mt19937 gen(rd());
    vector<thread> threads;
    for (int32_t i = 0; i < 50; i++) {
        int32_t randNumber = gen() % 100;
        if (randNumber < 25) {
            User user("WRITER");
            if (priority == 1) {
                threads.emplace_back(thread(solutionPriorityRead, ref(user), ref(counterUsers), i, ref(mtx)));
            }
            if (priority == 2) {
                threads.emplace_back(thread(solutionPriorityWrite, ref(user), ref(counterUsers), i, ref(mtx)));
            }
        }
        else {
            User user("READER");
            if (priority == 1) {
                threads.emplace_back(thread(solutionPriorityRead, ref(user), ref(counterUsers), i, ref(mtx)));
            }
            if (priority == 2) {
                threads.emplace_back(thread(solutionPriorityWrite, ref(user), ref(counterUsers), i, ref(mtx)));
            }
        }
    }
    for (thread& t: threads) {
        t.join();
    }
}

void prioritySelection() {
    cout << "Выберите приоритет\n [1] - Читатели\n [2] - Писатели\n>" << endl;
    int32_t inputData;
    cin >> inputData;
    if (inputData == 1) createThreads(1);
    else if (inputData == 2) createThreads(2);
    else throw runtime_error("Incorrect input command"); 
}

int main() {
    setlocale(LC_ALL, "RUSSIAN");
    menu();
    string inputCommand;
    cin >> inputCommand;
    try {
        if (inputCommand == "1") choosePrimitive();
        else if (inputCommand == "2") enterInputDataToSearchShops();
        else if (inputCommand == "3") prioritySelection();
        else if (inputCommand == "0") return 0;
        else throw runtime_error("Incorrect input command");
    }
    catch (exception &e) {
        cout << e.what() << endl;
    }
}
