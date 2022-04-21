// VR_Library.h


//is glew init needed here? maybe in the future?
#pragma once
#include "openvr.h"
#include <vector>
#include <memory>

struct vr_error {
	vr_error() {}
	vr_error(bool b) { success = b; }
	bool success;
};

enum vr_event_type {
	CONTROLLER_PRESS,
	HMD_POSITION,
	CONTROLLER,
	CONTROLLER_ACTIVATED
};

struct controller {
	vr::VRInputValueHandle_t m_source = vr::k_ulInvalidInputValueHandle;
	vr::VRActionHandle_t m_actionPose = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionHaptic = vr::k_ulInvalidActionHandle;
	vr::HmdMatrix34_t m_rmat4Pose;
	std::string m_sRenderModelName;
	bool m_bShowController;

	vr::RenderModel_t* pModel;
	vr::RenderModel_TextureMap_t* pTexture;

	bool b_selectButton_pressed = false;
	bool b_menuButton_pressed = false;
	bool b_touchpadButton_pressed = false;
	bool b_gripButton_pressed = false;
	float m_trackpadPositionX;
	float m_trackpadPositionY;
};

struct vr_event {
	vr_event_type type;
};

struct hmd_position : vr_event {
	hmd_position(vr::HmdMatrix34_t mat) { HMD_position = mat; }
	vr::HmdMatrix34_t HMD_position;
	
};

struct controller_activated : vr_event {
	controller_activated(controller* cont) { p_controller = cont; }
	controller* p_controller;
};

struct controller_press : vr_event {
	controller_press(bool selectButton, bool menuButton,
		bool touchpadButton, bool gripButton, float trackpadPositionx,
		float trackpadPositiony){
		selectButton_pressed = selectButton;
		menuButton_pressed = menuButton;
		touchpadButton_pressed = touchpadButton;
		gripButton_pressed = gripButton;
		trackpadPositionX = trackpadPositionx;
		trackpadPositionY = trackpadPositiony;
	}
	bool selectButton_pressed;
	bool menuButton_pressed;
	bool touchpadButton_pressed;
	bool gripButton_pressed;
	float trackpadPositionX;
	float trackpadPositionY;

};

class vr_library {
public:

	struct framebuffer_info {
		uint32_t render_width;
		uint32_t render_height;
		framebuffer_info() {}
		framebuffer_info(uint32_t w, uint32_t h) { render_width = w; render_height = h; }
	};


	vr::IVRSystem* m_pHMD;

	std::vector<std::unique_ptr<vr_event>> vr_events;


	vr_error init();

	vr_error update();

	void submit_frame(uint32_t left_eye_texture, uint32_t right_eye_texture) const;
	void submit_frame_overlay(uint32_t overlay_texture) const;

	void toggle_overlay(bool show) const;

	vr_library::framebuffer_info get_framebuffer_info() const;

	vr::HmdMatrix34_t m_mat4eyePosLeft;
	vr::HmdMatrix34_t m_mat4eyePosRight;
	vr::HmdMatrix44_t m_mat4ProjectionLeft;
	vr::HmdMatrix44_t m_mat4ProjectionRight;


	vr::HmdMatrix34_t m_mat4HMDPose;
	

	controller rlHand[2];

	int m_overlayWidth = 800;
	int m_overlayHeight = 500;


	//overlay controller information
	float m_laserPosX;
	float m_laserPosY;
	bool m_laserMouseTriggered;
	float m_laserMouseWheel;

	void activateOverlayLaser();
private:

	

	vr_error init_overlay();

	vr::VROverlayHandle_t m_ulOverlayHandle;
	
	
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	vr::HmdMatrix34_t m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	void update_hmd_matrix_pose();
	vr_error handle_input();

	void update_controller_state();
	void update_button_states();
	void update_overlay_events();


	enum EHand
	{
		Left = 0,
		Right = 1,
	};

	vr::VRActionSetHandle_t m_actionsetDemo = vr::k_ulInvalidActionSetHandle;

	vr::VRActionHandle_t m_action_AnalogInput = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_action_selectButton = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_action_GripButton = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_action_TouchpadButton = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_action_MenuButton = vr::k_ulInvalidActionHandle;

	void find_or_load_render_model(const char* pchRenderModelName, EHand ehand);

	

	vr::HmdMatrix34_t getProjectionEyeMatrix(vr::Hmd_Eye nEye);

	
};

