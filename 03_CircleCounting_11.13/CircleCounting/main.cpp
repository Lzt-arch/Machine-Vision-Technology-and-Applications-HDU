#include "ExerciseIO.hpp"

int wmain(int argc, wchar_t* argv[]) {
    return mv::start(argc, argv, [](const mv::Run& run) {
        const cv::Mat image = run.read(L"lt3.png");
        cv::Mat gray, smooth;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        cv::medianBlur(gray, smooth, 7);

        // Radius limits target the four small holes in LT3, excluding the large partial rings.
        const double scale = std::min(gray.cols, gray.rows) / 614.0;
        std::vector<cv::Vec3f> circles;
        cv::HoughCircles(smooth, circles, cv::HOUGH_GRADIENT, 1.2, 48 * scale, 100, 28,
                         std::max(1, cvRound(17 * scale)), std::max(2, cvRound(53 * scale)));
        std::sort(circles.begin(), circles.end(), [](const cv::Vec3f& a, const cv::Vec3f& b) {
            return a[1] < b[1];
        });

        cv::Mat annotated = image.clone();
        for (size_t i = 0; i < circles.size(); ++i) {
            const cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            const int radius = cvRound(circles[i][2]);
            cv::circle(annotated, center, radius, {0, 255, 0}, 2, cv::LINE_AA);
            cv::circle(annotated, center, 3, {0, 0, 255}, cv::FILLED);
            mv::label(annotated, std::to_string(i + 1), center + cv::Point(7, -6));
        }
        std::cout << "Circles: " << circles.size() << '\n';
        const std::string total = "Total circles: " + std::to_string(circles.size());
        const auto result = mv::withCountBanner(annotated, total);
        run.save("annotated.png", result);
        run.show("Circle counting | LT3", result);
    });
}
