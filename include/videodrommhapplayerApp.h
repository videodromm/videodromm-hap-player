/*
Copyright (c) 2010-2015, Paul Houx - All rights reserved.
This code is intended for use with the Cinder C++ library: http://libcinder.org

This file is part of Cinder-Warping.

Cinder-Warping is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Cinder-Warping is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cinder-Warping.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"

#include "Warp.h"
// Settings
#include "VDSettings.h"
// Session
#include "VDSession.h"
// Utils
#include "VDUtils.h"
// Audio
#include "VDAudio.h"
// Animation
#include "VDAnimation.h"
// shaders
#include "VDShaders.h"
// Message router
#include "VDRouter.h"
// fbo
#include "VDFbo.h"
// hap
#include "MovieHap.h"

using namespace ci;
using namespace ci::app;
using namespace ph::warping;
using namespace std;
using namespace VideoDromm;


#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))

class videodrommhapplayerApp : public App {

public:
	static void					prepare(Settings *settings);

	void						setup() override;
	void						cleanup() override;
	void						update() override;
	void						draw() override;

	void						resize() override;

	void						mouseMove(MouseEvent event) override;
	void						mouseDown(MouseEvent event) override;
	void						mouseDrag(MouseEvent event) override;
	void						mouseUp(MouseEvent event) override;

	void						keyDown(KeyEvent event) override;
	void						keyUp(KeyEvent event) override;

	void						updateWindowTitle();
	void						fileDrop(FileDropEvent event) override;
	void						loadMovieFile(const fs::path &path);


private:
	// Settings
	VDSettingsRef				mVDSettings;
	// Session
	VDSessionRef				mVDSession;
	// Utils
	VDUtilsRef					mVDUtils;
	// Audio
	VDAudioRef					mVDAudio;
	// Animation
	VDAnimationRef				mVDAnimation;
	// Shaders
	VDShadersRef				mVDShaders;
	// Fbos
	vector<VDFboRef>			mVDFbos;
	// Message router
	VDRouterRef					mVDRouter;
	// shaders
	gl::GlslProgRef				aShader;

	fs::path					mSettings;

	WarpList					mWarps;
	bool						mWaveDelay;
	bool						mMovieDelay;
	bool						mFadeInDelay;
	bool						mFadeOutDelay;
	gl::TextureRef				mImage;
	Area						mSrcArea;
	// fbo
	void						renderSceneToFbo();
	gl::FboRef					mRenderFbo;
	// hap
	gl::TextureRef				mInfoTexture;
	qtime::MovieGlHapRef		mMovie;
	bool						mLoopVideo;
};
