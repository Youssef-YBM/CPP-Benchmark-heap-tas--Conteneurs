#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <limits>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// Les trois classes ci-dessous exposent le meme tableau indexe, avec
// des couts d'acces differents selon le conteneur utilise.
class VectorStore {
public:
    std::size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    void reserve(std::size_t count) { data.reserve(count); }
    int& at(std::size_t index) { return data.at(index); }
    const int& at(std::size_t index) const { return data.at(index); }
    void pushBack(int value) { data.push_back(value); }
    void popBack() { data.pop_back(); }
    void clear() { data.clear(); }

private:
    std::vector<int> data;
};

class ListStore {
public:
    std::size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    void reserve(std::size_t) {}

    int& at(std::size_t index) {
        return *std::next(data.begin(), static_cast<std::ptrdiff_t>(index));
    }

    const int& at(std::size_t index) const {
        return *std::next(data.begin(), static_cast<std::ptrdiff_t>(index));
    }

    void pushBack(int value) { data.push_back(value); }
    void popBack() { data.pop_back(); }
    void clear() { data.clear(); }

private:
    std::list<int> data;
};

class MapStore {
public:
    std::size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    void reserve(std::size_t) {}
    int& at(std::size_t index) { return data.at(index); }
    const int& at(std::size_t index) const { return data.at(index); }

    void pushBack(int value) {
        data.emplace(data.size(), value);
    }

    void popBack() {
        data.erase(data.size() - 1);
    }

    void clear() { data.clear(); }

private:
    std::map<std::size_t, int> data;
};

// Tas generique : le comparateur determine si la racine est le minimum ou maximum.
template <typename Store, typename Compare>
class BinaryHeap {
public:
    std::size_t size() const { return values.size(); }
    bool empty() const { return values.empty(); }

    void reserve(std::size_t count) { values.reserve(count); }

    void insert(int value) {
        values.pushBack(value);
        siftUp(values.size() - 1);
    }

    int top() const {
        if (empty()) {
            throw std::runtime_error("Le tas est vide.");
        }
        return values.at(0);
    }

    int minimum() const { return top(); }
    int maximum() const { return top(); }

    bool contains(int value) const {
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (values.at(i) == value) {
                return true;
            }
        }
        return false;
    }

    bool erase(int value) {
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (values.at(i) == value) {
                eraseAt(i);
                return true;
            }
        }
        return false;
    }

    int extractTop() {
        const int result = top();
        eraseAt(0);
        return result;
    }

    int removeMinimum() { return extractTop(); }
    int removeMaximum() { return extractTop(); }

    std::string toString() const {
        std::ostringstream output;
        output << "[";
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i != 0) {
                output << ", ";
            }
            output << values.at(i);
        }
        output << "]";
        return output.str();
    }

private:
    Store values;
    Compare compare;

    void siftUp(std::size_t index) {
        while (index > 0) {
            const std::size_t parent = (index - 1) / 2;
            if (!compare(values.at(index), values.at(parent))) {
                break;
            }
            std::swap(values.at(parent), values.at(index));
            index = parent;
        }
    }

    void siftDown(std::size_t index) {
        while (index < values.size()) {
            const std::size_t left = 2 * index + 1;
            const std::size_t right = left + 1;
            std::size_t best = index;

            if (left < values.size() && compare(values.at(left), values.at(best))) {
                best = left;
            }
            if (right < values.size() && compare(values.at(right), values.at(best))) {
                best = right;
            }
            if (best == index) {
                break;
            }
            std::swap(values.at(index), values.at(best));
            index = best;
        }
    }

    void eraseAt(std::size_t index) {
        const std::size_t last = values.size() - 1;
        if (index != last) {
            values.at(index) = values.at(last);
        }
        values.popBack();

        if (index < values.size()) {
            if (index > 0 && compare(values.at(index), values.at((index - 1) / 2))) {
                siftUp(index);
            } else {
                siftDown(index);
            }
        }
    }
};

template <typename Store>
using MinHeap = BinaryHeap<Store, std::less<int>>;

template <typename Store>
using MaxHeap = BinaryHeap<Store, std::greater<int>>;

using HeapVector = MinHeap<VectorStore>;
using HeapList = MinHeap<ListStore>;
using HeapMap = MinHeap<MapStore>;
using HeapMaxVector = MaxHeap<VectorStore>;
using HeapMaxList = MaxHeap<ListStore>;
using HeapMaxMap = MaxHeap<MapStore>;

// Variante STL : les algorithmes standard manipulent directement un vector.
template <typename Compare>
class STLHeap {
public:
    std::size_t size() const { return values.size(); }
    bool empty() const { return values.empty(); }

    void reserve(std::size_t count) { values.reserve(count); }

    void insert(int value) {
        values.push_back(value);
        std::push_heap(values.begin(), values.end(), compare);
    }

    int top() const {
        if (empty()) {
            throw std::runtime_error("Le tas est vide.");
        }
        return values.front();
    }

    int minimum() const { return top(); }
    int maximum() const { return top(); }

    bool contains(int value) const {
        return std::find(values.begin(), values.end(), value) != values.end();
    }

    bool erase(int value) {
        const auto position = std::find(values.begin(), values.end(), value);
        if (position == values.end()) {
            return false;
        }
        values.erase(position);
        std::make_heap(values.begin(), values.end(), compare);
        return true;
    }

    int extractTop() {
        const int result = top();
        std::pop_heap(values.begin(), values.end(), compare);
        values.pop_back();
        return result;
    }

    int removeMinimum() { return extractTop(); }
    int removeMaximum() { return extractTop(); }

    std::string toString() const {
        std::ostringstream output;
        output << "[";
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i != 0) {
                output << ", ";
            }
            output << values[i];
        }
        output << "]";
        return output.str();
    }

private:
    std::vector<int> values;
    Compare compare;
};

using STLMinHeap = STLHeap<std::greater<int>>;
using STLMaxHeap = STLHeap<std::less<int>>;

struct BenchmarkResult {
    std::string heapType;
    std::string containerName;
    double insertionMs = 0.0;
    double extractionMs = 0.0;
};

template <typename Function>
double measureMilliseconds(Function function) {
    const auto start = std::chrono::steady_clock::now();
    function();
    const auto finish = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

template <typename Heap>
BenchmarkResult runBenchmark(
    const std::string& containerName,
    const std::string& heapType,
    const std::vector<int>& inputValues) {
    Heap heap;
    BenchmarkResult result;
    result.heapType = heapType;
    result.containerName = containerName;

    result.insertionMs = measureMilliseconds([&]() {
        heap.reserve(inputValues.size());
        for (int value : inputValues) {
            heap.insert(value);
        }
    });

    result.extractionMs = measureMilliseconds([&]() {
        while (!heap.empty()) {
            heap.extractTop();
        }
    });

    return result;
}

bool hasEnoughMemoryForBenchmark(std::size_t count) {
#ifdef _WIN32
    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (!GlobalMemoryStatusEx(&memoryStatus)) {
        throw std::runtime_error("Impossible de verifier la memoire disponible.");
    }

    constexpr std::size_t reserveBytes = 512ULL * 1024ULL * 1024ULL;
    const unsigned long long requiredBytes =
        static_cast<unsigned long long>(count) * sizeof(int) * 2ULL + reserveBytes;
    return memoryStatus.ullAvailPhys >= requiredBytes;
#else
    (void)count;
    return true;
#endif
}

void writeHeapLine(std::ofstream& output, const std::string& name, const std::string& heap) {
    output << name << " : " << heap << '\n';
}

template <typename Heap>
void writeInteractiveHeap(std::ofstream& output, const std::string& name, const Heap& heap) {
    writeHeapLine(output, name, heap.toString());
}

void saveInteractiveHeaps(
    const HeapVector& vectorHeap,
    const HeapList& listHeap,
    const HeapMap& mapHeap,
    const STLMinHeap& stlHeap,
    const HeapMaxVector& maxVectorHeap,
    const HeapMaxList& maxListHeap,
    const HeapMaxMap& maxMapHeap,
    const STLMaxHeap& stlMaxHeap) {
    std::ofstream output("output.txt", std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Impossible d'ouvrir output.txt en ecriture.");
    }

    output << "Affichage des tas min et max (ordre interne du tas)\n\n";
    writeInteractiveHeap(output, "min maison/vector", vectorHeap);
    writeInteractiveHeap(output, "min maison/list", listHeap);
    writeInteractiveHeap(output, "min maison/map", mapHeap);
    writeInteractiveHeap(output, "min STL/vector", stlHeap);
    writeInteractiveHeap(output, "max maison/vector", maxVectorHeap);
    writeInteractiveHeap(output, "max maison/list", maxListHeap);
    writeInteractiveHeap(output, "max maison/map", maxMapHeap);
    writeInteractiveHeap(output, "max STL/vector", stlMaxHeap);
    output << "\nLa racine contient le minimum ou le maximum selon le type de tas.\n";
}

void saveBenchmarkResults(
    std::size_t count,
    const std::vector<BenchmarkResult>& results) {
    std::ofstream output("output.txt", std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Impossible d'ouvrir output.txt en ecriture.");
    }

    output << "BENCHMARK DES TAS MIN ET MAX\n"
           << "Nombre d'elements inseres : " << count << "\n"
           << "Comparaison de l'insertion et de l'extraction de la racine.\n"
           << "Les tas de chaque type utilisent les memes valeurs aleatoires.\n"
           << "Les temps n'incluent ni la generation des valeurs ni ce fichier.\n"
           << "Les tas maison sur list et map sont limites a 5000 elements.\n\n";

    for (const BenchmarkResult& result : results) {
        output << "=== Tas " << result.heapType << " - " << result.containerName << " ===\n"
               << std::fixed << std::setprecision(6)
               << "Insertion (" << count << " elements) : "
               << result.insertionMs << " ms\n"
               << "Extraction de toutes les racines (" << count
               << " extractions) : " << result.extractionMs << " ms\n"
               << "Temps moyen par extraction : ";
        if (count == 0) {
            output << "n/a\n";
        } else {
            output << result.extractionMs * 1000.0 / count << " us\n";
        }
        output << '\n';
    }
}

int readInteger(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        int value = 0;
        if (std::cin >> value) {
            return value;
        }
        if (std::cin.eof()) {
            throw std::runtime_error("Fin de la saisie.");
        }
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "Saisie invalide : entrez un nombre entier.\n";
    }
}

void runInteractiveMenu() {
    HeapVector vectorHeap;
    HeapList listHeap;
    HeapMap mapHeap;
    STLMinHeap stlHeap;
    HeapMaxVector maxVectorHeap;
    HeapMaxList maxListHeap;
    HeapMaxMap maxMapHeap;
    STLMaxHeap stlMaxHeap;

    while (true) {
        std::cout << "\n===== MENU - TAS MIN ET MAX =====\n"
                  << "1. Inserer une valeur dans les huit tas\n"
                  << "2. Rechercher une valeur dans les huit tas\n"
                  << "3. Supprimer une valeur des huit tas\n"
                  << "4. Extraire les racines des tas min et max\n"
                  << "5. Afficher les tas dans output.txt\n"
                  << "6. Lancer le benchmark aleatoire\n"
                  << "0. Quitter\n";

        const int choice = readInteger("Votre choix : ");
        if (choice == 0) {
            return;
        }

        if (choice == 1) {
            const int value = readInteger("Valeur a inserer : ");
            vectorHeap.insert(value);
            listHeap.insert(value);
            mapHeap.insert(value);
            stlHeap.insert(value);
            maxVectorHeap.insert(value);
            maxListHeap.insert(value);
            maxMapHeap.insert(value);
            stlMaxHeap.insert(value);
            std::cout << "Valeur inseree dans les huit tas.\n";
        } else if (choice == 2) {
            const int value = readInteger("Valeur a rechercher : ");
            std::cout << "vector : " << (vectorHeap.contains(value) ? "trouvee" : "absente") << '\n'
                      << "list   : " << (listHeap.contains(value) ? "trouvee" : "absente") << '\n'
                      << "map    : " << (mapHeap.contains(value) ? "trouvee" : "absente") << '\n'
                      << "STL    : " << (stlHeap.contains(value) ? "trouvee" : "absente") << '\n'
                      << "max vector : " << (maxVectorHeap.contains(value) ? "trouvee" : "absente") << '\n'
                      << "max list   : " << (maxListHeap.contains(value) ? "trouvee" : "absente") << '\n'
                      << "max map    : " << (maxMapHeap.contains(value) ? "trouvee" : "absente") << '\n'
                      << "max STL    : " << (stlMaxHeap.contains(value) ? "trouvee" : "absente") << '\n';
        } else if (choice == 3) {
            const int value = readInteger("Valeur a supprimer : ");
            std::cout << "vector : " << (vectorHeap.erase(value) ? "supprimee" : "absente") << '\n'
                      << "list   : " << (listHeap.erase(value) ? "supprimee" : "absente") << '\n'
                      << "map    : " << (mapHeap.erase(value) ? "supprimee" : "absente") << '\n'
                      << "STL    : " << (stlHeap.erase(value) ? "supprimee" : "absente") << '\n'
                      << "max vector : " << (maxVectorHeap.erase(value) ? "supprimee" : "absente") << '\n'
                      << "max list   : " << (maxListHeap.erase(value) ? "supprimee" : "absente") << '\n'
                      << "max map    : " << (maxMapHeap.erase(value) ? "supprimee" : "absente") << '\n'
                      << "max STL    : " << (stlMaxHeap.erase(value) ? "supprimee" : "absente") << '\n';
        } else if (choice == 4) {
            if (vectorHeap.empty() || maxVectorHeap.empty()) {
                std::cout << "Les tas sont vides : aucune racine a extraire.\n";
                continue;
            }
            std::cout << "Minimums extraits : vector=" << vectorHeap.removeMinimum()
                      << ", list=" << listHeap.removeMinimum()
                      << ", map=" << mapHeap.removeMinimum()
                      << ", STL=" << stlHeap.removeMinimum() << '\n'
                      << "Maximums extraits : vector=" << maxVectorHeap.removeMaximum()
                      << ", list=" << maxListHeap.removeMaximum()
                      << ", map=" << maxMapHeap.removeMaximum()
                      << ", STL=" << stlMaxHeap.removeMaximum() << '\n';
        } else if (choice == 5) {
            saveInteractiveHeaps(
                vectorHeap, listHeap, mapHeap, stlHeap,
                maxVectorHeap, maxListHeap, maxMapHeap, stlMaxHeap);
            std::cout << "Les huit tas ont ete ecrits dans output.txt.\n";
        } else if (choice == 6) {
            const int requestedCount = readInteger("Nombre d'elements a tester (0 a 1000000000) : ");
            if (requestedCount < 0 || requestedCount > 1000000000) {
                std::cout << "Le nombre doit etre compris entre 0 et 1000000000.\n";
                continue;
            }

            const std::size_t count = static_cast<std::size_t>(requestedCount);
            if (!hasEnoughMemoryForBenchmark(count)) {
                const double requiredGiB =
                    (static_cast<double>(count) * sizeof(int) * 2.0
                     + 512.0 * 1024.0 * 1024.0) / (1024.0 * 1024.0 * 1024.0);
                std::cout << "Memoire disponible insuffisante. Cette mesure demande environ "
                          << std::fixed << std::setprecision(2) << requiredGiB
                          << " Gio libres (valeurs + tas vector + marge).\n";
                continue;
            }

            std::mt19937 generator(std::random_device{}());
            std::uniform_int_distribution<int> distribution(
                1, std::numeric_limits<int>::max());

            std::vector<int> values;
            values.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                values.push_back(distribution(generator));
            }

            std::vector<BenchmarkResult> results;
            results.push_back(runBenchmark<HeapVector>("Maison / vector", "min", values));
            results.push_back(runBenchmark<STLMinHeap>("STL / vector", "min", values));
            results.push_back(runBenchmark<HeapMaxVector>("Maison / vector", "max", values));
            results.push_back(runBenchmark<STLMaxHeap>("STL / vector", "max", values));
            if (count <= 5000) {
                results.push_back(runBenchmark<HeapList>("Maison / list", "min", values));
                results.push_back(runBenchmark<HeapMap>("Maison / map", "min", values));
                results.push_back(runBenchmark<HeapMaxList>("Maison / list", "max", values));
                results.push_back(runBenchmark<HeapMaxMap>("Maison / map", "max", values));
            } else {
                std::cout << "Pour N > 5000, list et map sont ignores : "
                             "leur acces indexe rend le tas trop lent.\n";
            }
            saveBenchmarkResults(count, results);

            std::cout << "\nBenchmark termine. Resultats enregistres dans output.txt\n"
                      << std::fixed << std::setprecision(6);
            for (const BenchmarkResult& result : results) {
                std::cout << result.containerName
                          << " : insertion " << result.insertionMs
                          << " ms, extraction des " << count << " racines "
                          << result.extractionMs << " ms";
                if (count != 0) {
                    std::cout << " (" << result.extractionMs * 1000.0 / count
                              << " us par extraction)";
                }
                std::cout << ".\n";
            }
        } else {
            std::cout << "Choix invalide.\n";
        }
    }
}

int main() {
    try {
        runInteractiveMenu();
    } catch (const std::exception& error) {
        std::cerr << "\nErreur : " << error.what() << '\n';
        return 1;
    }
    return 0;
}
