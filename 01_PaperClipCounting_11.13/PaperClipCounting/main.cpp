#include "ExerciseIO.hpp"

int wmain(int argc, wchar_t* argv[]) {
    return mv::start(argc, argv, [](const mv::Run& run) {
        const cv::Mat image = run.read(L"lt2.png");
        cv::Mat gray, smooth, binary, labels, stats, centers;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(gray, smooth, {5, 5}, 0);
        cv::threshold(smooth, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
        cv::morphologyEx(binary, binary, cv::MORPH_CLOSE,
                         cv::getStructuringElement(cv::MORPH_ELLIPSE, {5, 5}));
        const int n = cv::connectedComponentsWithStats(binary, labels, stats, centers, 8);
        cv::Mat clean = cv::Mat::zeros(gray.size(), CV_8U);
        cv::Mat annotated = image.clone();
        int count = 0;
        for (int i = 1; i < n; ++i) {
            const int* row = stats.ptr<int>(i);
            const cv::Rect box(row[0], row[1], row[2], row[3]);
            // Reject the scan border and tiny disconnected marks.
            if (box.x <= 1 || box.y <= 1 || box.br().x >= image.cols - 1 ||
                box.br().y >= image.rows - 1 || row[cv::CC_STAT_AREA] < gray.total() * 0.003) continue;
            clean.setTo(255, labels == i);
            cv::rectangle(annotated, box, {0, 240, 0}, 2);
            mv::label(annotated, std::to_string(++count), box.tl() + cv::Point(5, 24));
        }
        std::cout << "Paper clips: " << count << '\n';
        run.save("binary.png", clean);
        const std::string total = "Total paper clips: " + std::to_string(count);
        const auto result = mv::withCountBanner(annotated, total);
        run.save("annotated.png", result);
        const auto comparison = mv::withCountBanner(mv::strip({clean, annotated}, 520), total);
        run.save("comparison.png", comparison);
        run.show("Paper clip counting | binary / labels", comparison);
    });
}
