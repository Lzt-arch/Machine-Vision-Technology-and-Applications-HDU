#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace mv {
namespace fs = std::filesystem;

inline fs::path executableDir() {
    std::wstring buffer(32768, L'\0');
    const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (n == 0 || n >= buffer.size()) throw std::runtime_error("Cannot locate executable directory.");
    buffer.resize(n);
    return fs::path(buffer).parent_path();
}

struct Run {
    fs::path assets, output, input, video;
    bool headless = false, camera = false;
    double gamma = 0.85;
    int cameraIndex = 0, kernel = 15;

    Run(int argc, wchar_t* argv[]) {
        const auto base = executableDir();
        assets = base / "assets";
        output = base / "Results";
        for (int i = 1; i < argc; ++i) {
            const std::wstring arg = argv[i];
            auto value = [&]() -> std::wstring {
                if (++i == argc) throw std::runtime_error("Missing option value.");
                return argv[i];
            };
            if (arg == L"--headless") headless = true;
            else if (arg == L"--input") input = value();
            else if (arg == L"--output") output = value();
            else if (arg == L"--video") video = value();
            else if (arg == L"--camera") camera = true;
            else if (arg == L"--camera-index") { camera = true; cameraIndex = std::stoi(value()); }
            else if (arg == L"--gamma") gamma = std::stod(value());
            else if (arg == L"--kernel") kernel = std::stoi(value());
            else throw std::runtime_error("Unknown option.");
        }
        if (camera && !video.empty()) throw std::runtime_error("Choose either --camera or --video.");
    }

    cv::Mat read(const wchar_t* defaultName, int flags = cv::IMREAD_COLOR) const {
        const auto path = input.empty() ? assets / defaultName : input;
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open input image.");
        const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), {});
        if (bytes.empty()) throw std::runtime_error("Input image is empty.");
        cv::Mat image = cv::imdecode(bytes, flags);
        if (image.empty()) throw std::runtime_error("Cannot decode input image.");
        return image;
    }

    void save(const char* name, const cv::Mat& image) const {
        fs::create_directories(output);
        const fs::path path = output / name;
        std::vector<unsigned char> bytes;
        if (!cv::imencode(path.extension().string(), image, bytes)) {
            throw std::runtime_error("Cannot encode output image.");
        }
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!file) throw std::runtime_error("Cannot write output image.");
    }

    void show(const char* title, const cv::Mat& image) const {
        if (headless) return;
        cv::namedWindow(title, cv::WINDOW_NORMAL);
        const double scale = std::min({1.0, 1250.0 / image.cols, 790.0 / image.rows});
        cv::resizeWindow(title, std::max(1, cvRound(image.cols * scale)), std::max(1, cvRound(image.rows * scale)));
        cv::imshow(title, image);
        // Poll visibility so either a key or closing the window ends the exercise.
        while (cv::waitKey(50) < 0 && cv::getWindowProperty(title, cv::WND_PROP_VISIBLE) >= 1) {}
        cv::destroyAllWindows();
    }
};

inline cv::Mat strip(const std::vector<cv::Mat>& images, int height = 380) {
    std::vector<cv::Mat> tiles;
    for (const auto& image : images) {
        cv::Mat color, tile;
        if (image.channels() == 1) cv::cvtColor(image, color, cv::COLOR_GRAY2BGR);
        else color = image;
        const int width = std::max(1, cvRound(color.cols * (static_cast<double>(height) / color.rows)));
        cv::resize(color, tile, {width, height}, 0, 0, cv::INTER_AREA);
        tiles.push_back(tile);
    }
    cv::Mat combined;
    cv::hconcat(tiles, combined);
    return combined;
}

inline void label(cv::Mat& image, const std::string& text, cv::Point point) {
    point.x = std::clamp(point.x, 0, std::max(0, image.cols - 35));
    point.y = std::clamp(point.y, std::min(20, image.rows - 1), image.rows - 1);
    cv::putText(image, text, point, cv::FONT_HERSHEY_SIMPLEX, 0.65, {0, 0, 0}, 4, cv::LINE_AA);
    cv::putText(image, text, point, cv::FONT_HERSHEY_SIMPLEX, 0.65, {0, 255, 0}, 2, cv::LINE_AA);
}

template <typename Exercise>
int start(int argc, wchar_t* argv[], Exercise run) {
    try {
        cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);
        run(Run(argc, argv));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
}
