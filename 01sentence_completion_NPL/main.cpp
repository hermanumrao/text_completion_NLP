#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::string file_reader(std::string folderPath) {

  // reading all the content from all the files
  std::string combinedText;
  int skip_flag = 0;

  for (const auto &entry : fs::directory_iterator(folderPath)) {
    if (entry.path().extension() == ".txt") {
      std::ifstream file(entry.path());
      if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
          if (line.find("-----") != std::string::npos)
            break;
          for (char &c : line)
            c = std::tolower(c);
          if (line.find("table of contents") != std::string::npos)
            skip_flag += 2;
          if (line.empty()) {
            if (skip_flag > 0)
              skip_flag -= 1;
            continue;
          }

          if ((skip_flag == 0 && line.find("chapter") == std::string::npos) &&
              (line.find("part") == std::string::npos) &&
              (line.find("arthur conan") == std::string::npos) &&
              (line.find("sherlock holmes") == std::string::npos))
            combinedText += line + "\n";
        }
        file.close();
      }
    }
  }
  return combinedText;
}

std::string clean_text(const std::string &input) {
  std::string result;
  int space_flag = 0;

  for (char c : input) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == ',' ||
        c == '?' || c == '-' || c == ' ' || c == '\n') {
      if (c == ' ')
        space_flag += 1;
      else
        space_flag = 0;
      if (c == '.')
        result += " <fullstop>";
      else if (c == ',')
        result += " <comma>";
      else if (c == '?')
        result += " <question>";
      else if (space_flag < 2)
        result += c;
    }
  }

  return result;
}

std::map<std::vector<std::string>, std::vector<std::string>>
build_ngram_model(const std::string &text, int n) {
  std::map<std::vector<std::string>, std::vector<std::string>> model;
  std::istringstream iss(text);
  std::vector<std::string> history(n - 1);
  std::string word;
  int cnt = 0;

  while (iss >> word) {
    if (cnt >= n - 1) {
      std::vector<std::string> key(history);
      model[key].push_back(word);
      history.erase((history.begin()));
      history.push_back(word);
    } else {
      history.push_back(word);
      ++cnt;
    }
  }
  return model;
}

std::string generate_sentence(
    const std::map<std::vector<std::string>, std::vector<std::string>>
        &ngram_model,
    const std::vector<std::string> &start_words, int length) {

  std::string sentence;
  std::vector<std::string> current_history = start_words;

  std::random_device rd;
  std::mt19937 gen(rd());

  for (int i = 0; i < length; ++i) {
    auto it = ngram_model.find(current_history);
    if (it == ngram_model.end())
      break;

    const auto &next_words = it->second;
    if (next_words.empty())
      break;

    std::uniform_int_distribution<> dis(0, next_words.size() - 1);
    std::string next_word = next_words[dis(gen)];

    sentence += next_word + " ";

    current_history.erase(current_history.begin());
    current_history.push_back(next_word);

    // Stop if punctuation is found
    if (next_word.find("<fullstop>") != std::string::npos ||
        next_word.find("<question>") != std::string::npos) {
      break;
    }
  }

  return sentence;
}

std::string fix_punctuation(const std::string &input) {
  std::string result = input;
  std::vector<std::string> targets = {" <fullstop>", " <comma>", "<question>"},
                           replacements = {".", ",", "?"};
  for (unsigned int i = 0; i < targets.size(); i++) {
    int pos = 0;
    while ((pos = result.find(targets[i], pos)) != std::string::npos) {
      result.replace(pos, targets[i].length(), replacements[i]);
    }
  }
  return result;
}

int main() {
  std::string folderPath = "../data/sherlock/"; // Replace with your folder path
  int ngram = 2;

  // reading all the content from all the files
  std::string text;
  text = file_reader(folderPath);

  // data cleaning to remove special charecters
  text = clean_text(text);

  // training of the model
  std::map<std::vector<std::string>, std::vector<std::string>> model;
  model = build_ngram_model(text, ngram);

  // input the start sentence
  std::cout << "please enter the start string:\n";
  std::string input;
  std::getline(std::cin, input);

  for (char &c : input) {
    if (std::isalpha(c))
      c = std::tolower(c);
  }
  std::vector<std::string> start_words;
  std::stringstream ss(input);
  std::string word;

  while (ss >> word) {
    start_words.push_back(word);
  }
  if (start_words.size() < 2 * (ngram - 1))
    std::cout << "please provide a longer sentence";
  else {
    while (start_words.size() > 2 * (ngram - 1)) {
      start_words.erase(start_words.begin());
    }
  }

  for (auto i : start_words)
    std::cout << i << ',';
  std::cout << std::endl;

  std::string sentence = generate_sentence(model, start_words, 40);

  // fix the sentence for punctuation
  sentence = fix_punctuation(sentence);

  // Output the combined text
  std::cout << input << ' ' << sentence << std::endl;

  return 0;
}
