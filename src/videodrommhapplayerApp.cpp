#include "videodrommhapplayerApp.h"

void videodrommhapplayerApp::prepare(Settings *settings)
{
	settings->setWindowSize(40, 10);
}

void videodrommhapplayerApp::setup()
{
	mWaveDelay = mMovieDelay = mFadeInDelay = mFadeOutDelay = true;
	// Settings
	mVDSettings = VDSettings::create();
	mVDSettings->mLiveCode = false;
	mVDSettings->mRenderThumbs = false;
	mVDSession = VDSession::create(mVDSettings);
	// Utils
	mVDUtils = VDUtils::create(mVDSettings);
	mVDUtils->getWindowsResolution();
	// Audio
	mVDAudio = VDAudio::create(mVDSettings);
	// Animation
	mVDAnimation = VDAnimation::create(mVDSettings, mVDSession);
	// Shaders
	mVDShaders = VDShaders::create(mVDSettings);
	// mix fbo at index 0
	mVDFbos.push_back(VDFbo::create(mVDSettings, "mix", mVDSettings->mFboWidth, mVDSettings->mFboHeight));

	gl::enableDepthWrite();
	gl::enableDepthRead();
	gl::disableBlending();

	// warping
	updateWindowTitle();
	disableFrameRate();

	// initialize warps
	mSettings = getAssetPath("") / mVDSettings->mAssetsPath / "warps.xml";
	if (fs::exists(mSettings)) {
		// load warp settings from file if one exists
		mWarps = Warp::readSettings(loadFile(mSettings));
	}
	else {
		// otherwise create a warp from scratch
		mWarps.push_back(WarpPerspectiveBilinear::create());
		mWarps.push_back(WarpPerspectiveBilinear::create());
	}

	mSrcArea = Area(0, 22, 398, 420);//Area(x1, y1, x2, y2);

	// render fbo
	gl::Fbo::Format fboFormat;
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing
	mRenderFbo = gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFormat.colorTexture());

	// hap
	mLoopVideo = false;
	// bpm
	setFrameRate(mVDSession->getTargetFps());

}
void videodrommhapplayerApp::cleanup()
{
	// save warp settings
	Warp::writeSettings(mWarps, writeFile(mSettings));
	mVDSettings->save();
	mVDSession->save();
}
void videodrommhapplayerApp::update()
{
	mVDAudio->update();
	mVDAnimation->update();


	updateWindowTitle();
}

// Render the scene into the FBO
void videodrommhapplayerApp::renderSceneToFbo()
{
	// this will restore the old framebuffer binding when we leave this function
	// on non-OpenGL ES platforms, you can just call mFbo->unbindFramebuffer() at the end of the function
	// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
	gl::ScopedFramebuffer fbScp(mRenderFbo);
	gl::clear(Color::black(), true);
	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp(ivec2(0), mRenderFbo->getSize());
	gl::color(Color::white());
	//gl::viewport(0, -200,1020,768);// getWindowSize());

	if (mMovie) {
		if (mMovie->isPlaying()) mMovie->draw();
	}
}
void videodrommhapplayerApp::draw()
{
	renderSceneToFbo();

	/***********************************************
	* mix 2 FBOs begin
	* first render the 2 frags to fbos (done before)
	* then use them as textures for the mix shader
	*/

	// draw using the mix shader
	mVDFbos[mVDSettings->mMixFboIndex]->getFboRef()->bindFramebuffer();
	//gl::setViewport(mVDFbos[mVDSettings->mMixFboIndex].fbo.getBounds());

	// clear the FBO
	gl::clear();
	gl::setMatricesWindow(mVDSettings->mFboWidth, mVDSettings->mFboHeight);

	aShader = mVDShaders->getMixShader();
	aShader->bind();
	aShader->uniform("iGlobalTime", mVDSettings->iGlobalTime);
	//20140703 aShader->uniform("iResolution", vec3(mVDSettings->mRenderResoXY.x, mVDSettings->mRenderResoXY.y, 1.0));
	aShader->uniform("iResolution", vec3(mVDSettings->mFboWidth, mVDSettings->mFboHeight, 1.0));
	aShader->uniform("iChannelResolution", mVDSettings->iChannelResolution, 4);
	aShader->uniform("iMouse", vec4(mVDSettings->mRenderPosXY.x, mVDSettings->mRenderPosXY.y, mVDSettings->iMouse.z, mVDSettings->iMouse.z));//iMouse =  Vec3i( event.getX(), mRenderHeight - event.getY(), 1 );
	aShader->uniform("iChannel0", 0);
	aShader->uniform("iChannel1", 1);
	aShader->uniform("iAudio0", 0);
	aShader->uniform("iFreq0", mVDSettings->iFreqs[0]);
	aShader->uniform("iFreq1", mVDSettings->iFreqs[1]);
	aShader->uniform("iFreq2", mVDSettings->iFreqs[2]);
	aShader->uniform("iFreq3", mVDSettings->iFreqs[3]);
	aShader->uniform("iChannelTime", mVDSettings->iChannelTime, 4);
	aShader->uniform("iColor", vec3(mVDSettings->controlValues[1], mVDSettings->controlValues[2], mVDSettings->controlValues[3]));// mVDSettings->iColor);
	aShader->uniform("iBackgroundColor", vec3(mVDSettings->controlValues[5], mVDSettings->controlValues[6], mVDSettings->controlValues[7]));// mVDSettings->iBackgroundColor);
	aShader->uniform("iSteps", (int)mVDSettings->controlValues[20]);
	aShader->uniform("iRatio", mVDSettings->controlValues[11]);//check if needed: +1;//mVDSettings->iRatio); 
	aShader->uniform("width", 1);
	aShader->uniform("height", 1);
	aShader->uniform("iRenderXY", mVDSettings->mRenderXY);
	aShader->uniform("iZoom", mVDSettings->controlValues[22]);
	aShader->uniform("iAlpha", mVDSettings->controlValues[4] * mVDSettings->iAlpha);
	aShader->uniform("iBlendmode", mVDSettings->iBlendMode);
	aShader->uniform("iChromatic", mVDSettings->controlValues[10]);
	aShader->uniform("iRotationSpeed", mVDSettings->controlValues[19]);
	aShader->uniform("iCrossfade", mVDSettings->controlValues[18]);
	aShader->uniform("iPixelate", mVDSettings->controlValues[15]);
	aShader->uniform("iExposure", mVDSettings->controlValues[14]);
	aShader->uniform("iDeltaTime", mVDAnimation->iDeltaTime);
	aShader->uniform("iFade", (int)mVDSettings->iFade);
	aShader->uniform("iToggle", (int)mVDSettings->controlValues[46]);
	aShader->uniform("iLight", (int)mVDSettings->iLight);
	aShader->uniform("iLightAuto", (int)mVDSettings->iLightAuto);
	aShader->uniform("iGreyScale", (int)mVDSettings->iGreyScale);
	aShader->uniform("iTransition", mVDSettings->iTransition);
	aShader->uniform("iAnim", mVDSettings->iAnim.value());
	aShader->uniform("iRepeat", (int)mVDSettings->iRepeat);
	aShader->uniform("iVignette", (int)mVDSettings->controlValues[47]);
	aShader->uniform("iInvert", (int)mVDSettings->controlValues[48]);
	aShader->uniform("iDebug", (int)mVDSettings->iDebug);
	aShader->uniform("iShowFps", (int)mVDSettings->iShowFps);
	aShader->uniform("iFps", mVDSettings->iFps);
	aShader->uniform("iTempoTime", mVDAnimation->iTempoTime);
	aShader->uniform("iGlitch", (int)mVDSettings->controlValues[45]);
	aShader->uniform("iTrixels", mVDSettings->controlValues[16]);
	aShader->uniform("iGridSize", mVDSettings->controlValues[17]);
	aShader->uniform("iBeat", mVDSettings->iBeat);
	aShader->uniform("iSeed", mVDSettings->iSeed);
	aShader->uniform("iRedMultiplier", mVDSettings->iRedMultiplier);
	aShader->uniform("iGreenMultiplier", mVDSettings->iGreenMultiplier);
	aShader->uniform("iBlueMultiplier", mVDSettings->iBlueMultiplier);
	aShader->uniform("iFlipH", mVDFbos[mVDSettings->mMixFboIndex]->isFlipH());
	aShader->uniform("iFlipV", mVDFbos[mVDSettings->mMixFboIndex]->isFlipV());
	aShader->uniform("iParam1", mVDSettings->iParam1);
	aShader->uniform("iParam2", mVDSettings->iParam2);
	aShader->uniform("iXorY", mVDSettings->iXorY);
	aShader->uniform("iBadTv", mVDSettings->iBadTv);

	mRenderFbo->getColorTexture()->bind(0);
	mRenderFbo->getColorTexture()->bind(1);
	gl::drawSolidRect(Rectf(0, 0, mVDSettings->mFboWidth, mVDSettings->mFboHeight));
	// stop drawing into the FBO
	mVDFbos[mVDSettings->mMixFboIndex]->getFboRef()->unbindFramebuffer();
	mRenderFbo->getColorTexture()->unbind();
	mRenderFbo->getColorTexture()->unbind();

	//aShader->unbind();
	//sTextures[5] = mVDFbos[mVDSettings->mMixFboIndex]->getTexture();

	//}
	/***********************************************
	* mix 2 FBOs end
	*/
	if (mFadeInDelay) {
		if (getElapsedFrames() > mVDSession->getFadeInDelay()) {
			mFadeInDelay = false;
			setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
			setWindowPos(ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY));
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 2.0f, EaseInCubic());
		}
	}
	if (mWaveDelay) {
		if (getElapsedFrames() > mVDSession->getWavePlaybackDelay()) {
			mWaveDelay = false;
			fs::path waveFile = getAssetPath("") / mVDSettings->mAssetsPath / mVDSession->getWaveFileName();
			mVDAudio->loadWaveFile(waveFile.string());
		}
	}
	if (mMovieDelay) {
		if (getElapsedFrames() > mVDSession->getMoviePlaybackDelay()) {
			mMovieDelay = false;
			fs::path movieFile = getAssetPath("") / mVDSettings->mAssetsPath / mVDSession->getMovieFileName();
			loadMovieFile(movieFile.string());
		}
	}
	if (mFadeOutDelay) {
		if (getElapsedFrames() > mVDSession->getEndFrame()) {
			mFadeOutDelay = false;
			timeline().apply(&mVDSettings->iAlpha, 1.0f, 0.0f, 2.0f, EaseInCubic());
		}
	}
	gl::clear(Color::black());
	gl::setMatricesWindow(toPixels(getWindowSize()));
	//gl::draw(mRenderFbo->getColorTexture());
	int i = 0;

	for (auto &warp : mWarps) {
			if (i % 2 == 0) {
				warp->draw(mVDFbos[mVDSettings->mMixFboIndex]->getTexture(), mSrcArea, warp->getBounds());//mVDUtils->getSrcAreaLeftOrTop());
			}
			else {
				warp->draw(mVDFbos[mVDSettings->mMixFboIndex]->getTexture(), mSrcArea, warp->getBounds());//mVDUtils->getSrcAreaRightOrBottom());
			}
		//warp->draw(mRenderFbo->getColorTexture(), mRenderFbo->getBounds());
		i++;
	}

}
void videodrommhapplayerApp::resize()
{
	// tell the warps our window has been resized, so they properly scale up or down
	Warp::handleResize(mWarps);
}

void videodrommhapplayerApp::mouseMove(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseMove(mWarps, event)) {
		// let your application perform its mouseMove handling here
		mVDSettings->controlValues[10] = event.getX() / mVDSettings->mRenderWidth;
		//mVDUtils->moveX1LeftOrTop(event.getX());
		//mVDUtils->moveY1LeftOrTop(event.getY());
	}
}

void videodrommhapplayerApp::mouseDown(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDown(mWarps, event)) {
		// let your application perform its mouseDown handling here
		mVDSettings->controlValues[45] = 1.0f;
	}
}
void videodrommhapplayerApp::mouseDrag(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDrag(mWarps, event)) {
		// let your application perform its mouseDrag handling here
	}
}

void videodrommhapplayerApp::mouseUp(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseUp(mWarps, event)) {
		// let your application perform its mouseUp handling here
		mVDSettings->controlValues[45] = 0.0f;
	}
}
void videodrommhapplayerApp::keyDown(KeyEvent event)
{
	fs::path moviePath;
	string fileName;

	// pass this key event to the warp editor first
	if (!Warp::handleKeyDown(mWarps, event)) {
		// warp editor did not handle the key, so handle it here
		if (!mVDAnimation->handleKeyDown(event)) {
			// Animation did not handle the key, so handle it here
			switch (event.getCode()) {

			case KeyEvent::KEY_ESCAPE:
				// quit the application
				quit();
				break;
			case KeyEvent::KEY_f:
				// toggle full screen
				setFullScreen(!isFullScreen());
				break;
			case KeyEvent::KEY_v:
				// toggle vertical sync
				//gl::enableVerticalSync(!gl::isVerticalSyncEnabled());
				mVDSettings->mSplitWarpV = !mVDSettings->mSplitWarpV;
				mVDUtils->splitWarp(mRenderFbo->getWidth(), mRenderFbo->getHeight());
				break;
			case KeyEvent::KEY_h:
				mVDSettings->mSplitWarpH = !mVDSettings->mSplitWarpH;
				mVDUtils->splitWarp(mRenderFbo->getWidth(), mRenderFbo->getHeight());
				break;
			case KeyEvent::KEY_r:
				// reset split the image
				mVDSettings->mSplitWarpV = false;
				mVDSettings->mSplitWarpH = false;
				mVDUtils->splitWarp(mRenderFbo->getWidth(), mRenderFbo->getHeight());
				break;
			case KeyEvent::KEY_w:
				// toggle warp edit mode
				Warp::enableEditMode(!Warp::isEditModeEnabled());
				break;
			/*case KeyEvent::KEY_c:
				mUseBeginEnd = !mUseBeginEnd;
				break;*/
			case KeyEvent::KEY_i:
				// toggle drawing a random region of the image
				if (mMovie) {
					if (mSrcArea.getWidth() != mMovie->getWidth() || mSrcArea.getHeight() != mMovie->getHeight())
						mSrcArea = mMovie->getBounds();
					else {
						int x1 = Rand::randInt(0, mMovie->getWidth() - 150);
						int y1 = Rand::randInt(0, mMovie->getHeight() - 150);
						int x2 = Rand::randInt(x1 + 150, mMovie->getWidth());
						int y2 = Rand::randInt(y1 + 150, mMovie->getHeight());
						mSrcArea = Area(x1, y1, x2, y2);
					}
				}
				break;
			case KeyEvent::KEY_o:
				// open hap video
				moviePath = getOpenFilePath();
				if (!moviePath.empty())
					loadMovieFile(moviePath);
				break;
			case KeyEvent::KEY_p:
				if (mMovie) mMovie->play();
				break;
			case KeyEvent::KEY_s:
				if (mMovie) mMovie->stop();
				break;
			case KeyEvent::KEY_SPACE:
				if (mMovie->isPlaying()) mMovie->stop(); else mMovie->play();
				break;
			case KeyEvent::KEY_l:
				mLoopVideo = !mLoopVideo;
				if (mMovie) mMovie->setLoop(mLoopVideo);
				break;
			case KeyEvent::KEY_a:
				fileName = "warps" + toString(getElapsedFrames()) + ".xml";
				mSettings = getAssetPath("") / mVDSettings->mAssetsPath / fileName;
				Warp::writeSettings(mWarps, writeFile(mSettings));
				mSettings = getAssetPath("") / mVDSettings->mAssetsPath / "warps.xml";
				break;
			}

		}
	}
}
void videodrommhapplayerApp::loadMovieFile(const fs::path &moviePath)
{
	try {
		mMovie.reset();
		// load up the movie, set it to loop, and begin playing
		mMovie = qtime::MovieGlHap::create(moviePath);
		mLoopVideo = (mMovie->getDuration() < 30.0f);
		mMovie->setLoop(mLoopVideo);
		mMovie->play();
	}
	catch (ci::Exception &e)
	{
		console() << string(e.what()) << std::endl;
		console() << "Unable to load the movie." << std::endl;
		mInfoTexture.reset();
	}
}

void videodrommhapplayerApp::fileDrop(FileDropEvent event)
{
	loadMovieFile(event.getFile(0));
}
void videodrommhapplayerApp::keyUp(KeyEvent event)
{
	// pass this key event to the warp editor first
	if (!Warp::handleKeyUp(mWarps, event)) {
		if (!mVDAnimation->handleKeyUp(event)) {
			// Animation did not handle the key, so handle it here
		}
	}
}

void videodrommhapplayerApp::updateWindowTitle()
{
	getWindow()->setTitle(to_string(getElapsedFrames()) + " " + to_string((int)getAverageFps()) + " fps Batchass Sky");
}

CINDER_APP(videodrommhapplayerApp, RendererGl, &videodrommhapplayerApp::prepare)
