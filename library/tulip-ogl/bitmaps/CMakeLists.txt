INSTALL(FILES fontb.ttf
              font.ttf
              fa-brands-400.ttf
              fa-brands-400.woff2
              fa-regular-400.ttf
              fa-regular-400.woff2
              fa-solid-900.ttf
              fa-solid-900.woff2
              materialdesignicons-webfont.ttf
              materialdesignicons-webfont.woff2
              halfCylinderTexture.png
              cylinderTexture.png
              roundTexture.png
	      backSlashStripesTexture.png
	      circleTexture.png
	      horizontalStripesTexture.png
	      verticalStripesTexture.png
	      slashStripesTexture.png
        DESTINATION ${TulipBitmapInstallDir}
        COMPONENT tulip_ogl)

INSTALL(DIRECTORY fonts
        COMPONENT tulip_ogl
        DESTINATION ${TulipBitmapInstallDir})
