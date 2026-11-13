#include "ExerciseIO.hpp"

int wmain(int argc, wchar_t* argv[]) {
    return mv::start(argc, argv, [](const mv::Run& run) {
        cv::Mat image = run.read(L"lt1.png");
        cv::Mat gray, smooth, binary;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(gray, smooth, {5, 5}, 0);
        const double threshold = cv::threshold(smooth, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::morphologyEx(binary, binary, cv::MORPH_CLOSE,
                         cv::getStructuringElement(cv::MORPH_ELLIPSE, {7, 7}));

        // Fill each exterior contour so dark markings within a coin are not holes.
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        cv::Mat filled = cv::Mat::zeros(binary.size(), CV_8U);
        cv::drawContours(filled, contours, -1, cv::Scalar(255), cv::FILLED);

        cv::Mat components, stats, centers;
        const int labels = cv::connectedComponentsWithStats(filled, components, stats, centers, 8);
        cv::Mat clean = cv::Mat::zeros(binary.size(), CV_8U);
        cv::Mat annotated = image.clone();
        int count = 0;
        for (int i = 1; i < labels; ++i) {
            const int* row = stats.ptr<int>(i);
            if (row[cv::CC_STAT_AREA] < gray.total() * 0.01) continue;
            cv::Rect box(row[0], row[1], row[2], row[3]);
            const double ratio = static_cast<double>(box.width) / box.height;
            if (ratio < 0.7 || ratio > 1.4) continue;
            clean.setTo(255, components == i);
            cv::rectangle(annotated, box, {20, 220, 30}, 2);
            mv::label(annotated, std::to_string(++count), box.tl() + cv::Point(6, 24));
        }
        std::cout << "Otsu threshold: " << threshold << "\nCoins: " << count << '\n';
        run.save("binary.png", clean);
        const std::string total = "Total coins: " + std::to_string(count);
        const auto result = mv::withCountBanner(annotated, total);
        run.save("annotated.png", result);
        const auto comparison = mv::withCountBanner(mv::strip({image, clean, annotated}), total);
        run.save("comparison.png", comparison);
        run.show("Coin counting | original / binary / labels", comparison);
    });
}
