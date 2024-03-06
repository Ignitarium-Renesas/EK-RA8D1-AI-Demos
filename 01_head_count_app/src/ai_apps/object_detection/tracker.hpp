#include <vector>
#include "DetectionResult.hpp"


static float hw_thresold = 1.3;

namespace arm {
namespace app {

class Tracker
{
  private:
    std::vector<object_detection::DetectionResult> tracks;
    int m_nextTrackId;
    int m_maxInactiveFrames;
    float m_track_IOU_thresold;

  public:
    Tracker(int maxInactiveFrames, float track_IOU_thresold)
        : m_nextTrackId(1), m_maxInactiveFrames(maxInactiveFrames), m_track_IOU_thresold(track_IOU_thresold)
    {
    }

    void update(std::vector<object_detection::DetectionResult> &results)
    {

        // Mark all tracks as inactive
    	for (uint32_t i = 0; i < tracks.size(); ++i) {
    		tracks[i].m_inactive_frames++;
    		tracks[i].m_is_matched = false;
    	}

        float max_track_iou = 0.0;
        int max_iou_index = -1;

        for (uint32_t i = 0; i < results.size(); i++) {
            bool matched = false;
            for (uint32_t j = 0; j < tracks.size(); j++) {
            	if(tracks[j].m_is_matched)
            		continue;
                bool hw_match = calculate_width_ratio(results[i], tracks[j]);
                if (hw_match) {
                    float track_iou = calculateIOU(results[i], tracks[j]);
                    if (track_iou>max_track_iou){
                        max_track_iou=track_iou;
                        max_iou_index = i;
                    }

                    if (max_track_iou > m_track_IOU_thresold) {

                        int32_t box_id_num = tracks[j].m_box_id_num;
                        int32_t box_id_count = tracks[j].m_box_id_count + 1;
                        tracks[j] = results[max_iou_index];
                        tracks[j].m_inactive_frames = 0;
                        tracks[j].m_is_matched = true;
                        tracks[j].m_box_id_num = box_id_num;
                        tracks[j].m_box_id_count = box_id_count;
                        matched = true;
                        max_track_iou = 0.0;
                        
                        break;
                    }
                }
            }

            if (!matched) {
                // Create new track
            	object_detection::DetectionResult newTrack = results[i];
            	newTrack.m_box_id_num = m_nextTrackId;
            	newTrack.m_box_id_count = 0;
            	newTrack.m_inactive_frames = 0;
            	tracks.push_back(newTrack);
            	m_nextTrackId++;
            }
        }

        std::vector<object_detection::DetectionResult> tmp_tracks;
        // Remove tracks that have been inactive for too long
        for (uint32_t i = 0; i < tracks.size(); ++i) {
			if(tracks[i].m_inactive_frames < m_maxInactiveFrames){
				tmp_tracks.push_back(tracks[i]);
			}
		}
        tracks = tmp_tracks;
        results = tracks;
    }

    bool calculate_width_ratio(const object_detection::DetectionResult &box1, const object_detection::DetectionResult &box2)
    {
        int box1width = box1.m_w;
        int box2width = box2.m_w;

        float width_ratio = -1;
        if (box1width > box2width) {
            width_ratio = box1width / (float)box2width;
        } else {
            width_ratio = box2width / (float)box1width;
        }

        if (width_ratio < hw_thresold) {
            return true;
        } else {
            return false;
        }
    }

    float calculateIOU(const object_detection::DetectionResult &box1, const object_detection::DetectionResult &box2)
    {
        int xLeft = std::max(box1.m_x0, box2.m_x0);
		int yTop = std::min(box1.m_y0+box1.m_h, box2.m_y0+box2.m_h);
		int xRight = std::min(box1.m_x0+box1.m_w, box2.m_x0+box2.m_w);
		int yBottom = std::max(box1.m_y0, box2.m_y0);

        int intersectionArea = std::max(0, xRight - xLeft) * std::max(0, yTop - yBottom);
        int box1Area = box1.m_w * box1.m_h;
        int box2Area = box2.m_w * box2.m_h;

        float track_iou = (float)(intersectionArea) / (box1Area + box2Area - intersectionArea);
        return track_iou;
    }
};
} /* namespace app */
} /* namespace arm */
