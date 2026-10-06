#include "RagdollEngine/Render/RagdollRenderer.hpp"
#include "RagdollEngine/Physics/PhysicsUnits.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <cmath>
#include <algorithm>

namespace RagdollEngine {

RagdollRenderer::RagdollRenderer() = default;

void RagdollRenderer::drawLimbSegment(sf::RenderWindow& window, b2BodyId body, float halfWidthPixels, float halfHeightPixels, const sf::Color& color) {
    if (!b2Body_IsValid(body)) return;

    sf::Vector2f pos = PhysicsUnits::toPixels(b2Body_GetPosition(body));
    b2Rot rot = b2Body_GetRotation(body);
    float rad = b2Rot_GetAngle(rot);

    // Main capsule rectangle
    sf::RectangleShape rect(sf::Vector2f(halfWidthPixels * 2.0f, halfHeightPixels * 2.0f));
    rect.setOrigin(sf::Vector2f(halfWidthPixels, halfHeightPixels));
    rect.setPosition(pos);
    rect.setRotation(sf::radians(rad));
    rect.setFillColor(color);
    window.draw(rect);

    // Rounded end caps
    float sinA = std::sin(rad) * halfHeightPixels;
    float cosA = std::cos(rad) * halfHeightPixels;

    sf::CircleShape capTop(halfWidthPixels);
    capTop.setOrigin(sf::Vector2f(halfWidthPixels, halfWidthPixels));
    capTop.setFillColor(color);
    capTop.setPosition(sf::Vector2f(pos.x + sinA, pos.y - cosA));
    window.draw(capTop);

    sf::CircleShape capBottom(halfWidthPixels);
    capBottom.setOrigin(sf::Vector2f(halfWidthPixels, halfWidthPixels));
    capBottom.setFillColor(color);
    capBottom.setPosition(sf::Vector2f(pos.x - sinA, pos.y + cosA));
    window.draw(capBottom);
}

void RagdollRenderer::drawShoe(sf::RenderWindow& window, b2BodyId shinBody, int facingDir, const CharacterDefinition& character) {
    if (!b2Body_IsValid(shinBody)) return;

    sf::Vector2f pos = PhysicsUnits::toPixels(b2Body_GetPosition(shinBody));
    b2Rot rot = b2Body_GetRotation(shinBody);
    float rad = b2Rot_GetAngle(rot);

    float halfLen = RagdollSkeleton::SHIN_LEN * 0.5f;
    float ankleX = pos.x - std::sin(rad) * halfLen;
    float ankleY = pos.y + std::cos(rad) * halfLen;

    float dir = static_cast<float>(facingDir);
    float shoeLen = 17.5f;
    float shoeHeight = 9.0f;

    // Offset forward slightly so heel sits at ankle and toe points forward
    float shoeCenterX = ankleX + dir * 3.5f;
    float shoeCenterY = ankleY + 2.5f;

    // 1. Boot Collar (Combat Boots - Ellie)
    if (character.footwear == FootwearType::CombatBoots) {
        sf::RectangleShape collar(sf::Vector2f(7.5f, 6.5f));
        collar.setOrigin(sf::Vector2f(3.75f, 6.5f));
        collar.setPosition(sf::Vector2f(ankleX, ankleY + 1.5f));
        collar.setFillColor(character.shoeColor);
        collar.setOutlineColor(character.headOutlineColor);
        collar.setOutlineThickness(1.5f);
        window.draw(collar);
    }

    // 2. Main rounded shoe capsule (Flash cartoon bean shoe)
    sf::RectangleShape shoeBody(sf::Vector2f(shoeLen, shoeHeight));
    shoeBody.setOrigin(sf::Vector2f(shoeLen * 0.5f, shoeHeight * 0.5f));
    shoeBody.setPosition(sf::Vector2f(shoeCenterX, shoeCenterY));
    shoeBody.setFillColor(character.shoeColor);
    shoeBody.setOutlineColor(character.headOutlineColor);
    shoeBody.setOutlineThickness(1.5f);
    window.draw(shoeBody);

    // Rounded toe cap
    sf::CircleShape toe(shoeHeight * 0.5f);
    toe.setOrigin(sf::Vector2f(shoeHeight * 0.5f, shoeHeight * 0.5f));
    toe.setPosition(sf::Vector2f(shoeCenterX + dir * (shoeLen * 0.5f - 1.0f), shoeCenterY));
    toe.setFillColor(character.shoeColor);
    toe.setOutlineColor(character.headOutlineColor);
    toe.setOutlineThickness(1.5f);
    window.draw(toe);

    // Rounded heel cap
    sf::CircleShape heel(shoeHeight * 0.45f);
    heel.setOrigin(sf::Vector2f(shoeHeight * 0.45f, shoeHeight * 0.45f));
    heel.setPosition(sf::Vector2f(shoeCenterX - dir * (shoeLen * 0.5f - 1.0f), shoeCenterY));
    heel.setFillColor(character.shoeColor);
    heel.setOutlineColor(character.headOutlineColor);
    heel.setOutlineThickness(1.5f);
    window.draw(heel);

    // Flat black rubber sole
    sf::RectangleShape sole(sf::Vector2f(shoeLen + 3.0f, 2.2f));
    sole.setOrigin(sf::Vector2f((shoeLen + 3.0f) * 0.5f, 1.1f));
    sole.setPosition(sf::Vector2f(shoeCenterX, shoeCenterY + shoeHeight * 0.45f));
    sole.setFillColor(character.shoeSoleColor);
    window.draw(sole);

    // Specular shine for Golden Shoes (Reginald)
    if (character.footwear == FootwearType::GoldenShoes) {
        sf::RectangleShape glint(sf::Vector2f(shoeLen * 0.5f, 1.5f));
        glint.setOrigin(sf::Vector2f(shoeLen * 0.25f, 0.75f));
        glint.setPosition(sf::Vector2f(shoeCenterX, shoeCenterY - shoeHeight * 0.25f));
        glint.setFillColor(sf::Color(255, 250, 180, 220));
        window.draw(glint);
    }

    // Rocket thruster nozzle for CyberThrusters (RHM)
    if (character.footwear == FootwearType::CyberThrusters) {
        sf::CircleShape nozzle(2.5f);
        nozzle.setOrigin(sf::Vector2f(2.5f, 2.5f));
        nozzle.setPosition(sf::Vector2f(shoeCenterX - dir * (shoeLen * 0.5f + 1.0f), shoeCenterY + 1.0f));
        nozzle.setFillColor(sf::Color(60, 200, 255));
        window.draw(nozzle);
    }
}

void RagdollRenderer::drawHand(sf::RenderWindow& window, b2BodyId forearmBody, int facingDir, const CharacterDefinition& character) {
    if (!b2Body_IsValid(forearmBody)) return;

    sf::Vector2f pos = PhysicsUnits::toPixels(b2Body_GetPosition(forearmBody));
    b2Rot rot = b2Body_GetRotation(forearmBody);
    float rad = b2Rot_GetAngle(rot);

    float halfLen = RagdollSkeleton::FOREARM_LEN * 0.5f;
    float wristX = pos.x - std::sin(rad) * halfLen;
    float wristY = pos.y + std::cos(rad) * halfLen;

    float fistRadius = 5.8f;
    float dir = static_cast<float>(facingDir);

    // Classic cartoon white glove or cybernetic fist
    sf::CircleShape fist(fistRadius);
    fist.setOrigin(sf::Vector2f(fistRadius, fistRadius));
    fist.setPosition(sf::Vector2f(wristX, wristY));
    fist.setFillColor(character.gloveColor);
    fist.setOutlineColor(character.gloveOutlineColor);
    fist.setOutlineThickness(1.8f);
    window.draw(fist);

    // Thumb contour
    sf::CircleShape thumb(2.6f);
    thumb.setOrigin(sf::Vector2f(2.6f, 2.6f));
    thumb.setPosition(sf::Vector2f(wristX + dir * 3.0f, wristY - 1.2f));
    thumb.setFillColor(character.gloveColor);
    thumb.setOutlineColor(character.gloveOutlineColor);
    thumb.setOutlineThickness(1.2f);
    window.draw(thumb);

    // Cybernetic arm emitter for RHM
    if (character.handwear == HandwearType::CyberneticArm) {
        sf::CircleShape led(1.5f);
        led.setOrigin(sf::Vector2f(1.5f, 1.5f));
        led.setPosition(sf::Vector2f(wristX, wristY));
        led.setFillColor(sf::Color(80, 220, 255));
        window.draw(led);
    }
}

void RagdollRenderer::drawBackCosmetics(sf::RenderWindow& window, const sf::Vector2f& headPos, float headRad, int facingDir, const CharacterDefinition& character) {
    float dir = static_cast<float>(facingDir);

    // 1. Ellie Rose Cascading Red Spiky Hair (Back locks)
    if (character.headwear == HeadwearType::EllieHair) {
        sf::ConvexShape mane;
        mane.setPointCount(8);
        mane.setPoint(0, sf::Vector2f(headPos.x - dir * headRad * 0.35f, headPos.y - headRad * 0.95f));
        mane.setPoint(1, sf::Vector2f(headPos.x - dir * (headRad + 16.0f), headPos.y - headRad * 0.70f));
        mane.setPoint(2, sf::Vector2f(headPos.x - dir * (headRad + 7.0f), headPos.y - headRad * 0.30f));
        mane.setPoint(3, sf::Vector2f(headPos.x - dir * (headRad + 22.0f), headPos.y + headRad * 0.15f));
        mane.setPoint(4, sf::Vector2f(headPos.x - dir * (headRad + 8.0f), headPos.y + headRad * 0.55f));
        mane.setPoint(5, sf::Vector2f(headPos.x - dir * (headRad + 14.0f), headPos.y + headRad * 1.05f));
        mane.setPoint(6, sf::Vector2f(headPos.x - dir * headRad * 0.30f, headPos.y + headRad * 0.85f));
        mane.setPoint(7, sf::Vector2f(headPos.x - dir * headRad * 0.65f, headPos.y + headRad * 0.10f));

        mane.setFillColor(character.headwearColor);
        mane.setOutlineColor(character.headwearShadingColor);
        mane.setOutlineThickness(2.0f);
        window.draw(mane);

        // Shading contour
        sf::ConvexShape shade;
        shade.setPointCount(4);
        shade.setPoint(0, sf::Vector2f(headPos.x - dir * (headRad + 5.0f), headPos.y - headRad * 0.10f));
        shade.setPoint(1, sf::Vector2f(headPos.x - dir * (headRad + 18.0f), headPos.y + headRad * 0.15f));
        shade.setPoint(2, sf::Vector2f(headPos.x - dir * (headRad + 8.0f), headPos.y + headRad * 0.50f));
        shade.setPoint(3, sf::Vector2f(headPos.x - dir * headRad * 0.60f, headPos.y + headRad * 0.25f));
        shade.setFillColor(character.headwearShadingColor);
        window.draw(shade);
    }
    // 2. Charles Calvin Pilot Headphones (Far earcup)
    else if (character.headwear == HeadwearType::Headphones) {
        sf::CircleShape farCup(7.0f);
        farCup.setScale(sf::Vector2f(0.55f, 1.15f));
        farCup.setOrigin(sf::Vector2f(7.0f, 7.0f));
        farCup.setPosition(sf::Vector2f(headPos.x - dir * headRad * 0.85f, headPos.y));
        farCup.setFillColor(character.headwearColor);
        farCup.setOutlineColor(character.headwearAccentColor);
        farCup.setOutlineThickness(1.8f);
        window.draw(farCup);
    }
}

void RagdollRenderer::drawFrontCosmetics(sf::RenderWindow& window, const sf::Vector2f& headPos, float headRad, int facingDir, const CharacterDefinition& character) {
    float dir = static_cast<float>(facingDir);

    // 1. Ellie Rose Front Bang & Crown Tufts
    if (character.headwear == HeadwearType::EllieHair) {
        // Front bang
        sf::ConvexShape bang;
        bang.setPointCount(5);
        bang.setPoint(0, sf::Vector2f(headPos.x - dir * headRad * 0.30f, headPos.y - headRad * 0.90f));
        bang.setPoint(1, sf::Vector2f(headPos.x + dir * headRad * 0.55f, headPos.y - headRad * 0.65f));
        bang.setPoint(2, sf::Vector2f(headPos.x + dir * headRad * 0.82f, headPos.y - headRad * 0.05f));
        bang.setPoint(3, sf::Vector2f(headPos.x + dir * headRad * 0.40f, headPos.y - headRad * 0.25f));
        bang.setPoint(4, sf::Vector2f(headPos.x, headPos.y - headRad * 0.75f));

        bang.setFillColor(character.headwearColor);
        bang.setOutlineColor(character.headwearShadingColor);
        bang.setOutlineThickness(1.8f);
        window.draw(bang);

        // Crown tuft
        sf::ConvexShape crownTuft;
        crownTuft.setPointCount(3);
        crownTuft.setPoint(0, sf::Vector2f(headPos.x - dir * headRad * 0.15f, headPos.y - headRad * 0.95f));
        crownTuft.setPoint(1, sf::Vector2f(headPos.x - dir * headRad * 0.50f, headPos.y - headRad * 1.32f));
        crownTuft.setPoint(2, sf::Vector2f(headPos.x - dir * headRad * 0.55f, headPos.y - headRad * 0.85f));
        crownTuft.setFillColor(character.headwearColor);
        crownTuft.setOutlineColor(character.headwearShadingColor);
        crownTuft.setOutlineThickness(1.8f);
        window.draw(crownTuft);
    }
    // 2. Charles Calvin Pilot Headphones & Boom Mic
    else if (character.headwear == HeadwearType::Headphones) {
        // Padded Headband over crown
        sf::RectangleShape headband(sf::Vector2f(headRad * 1.9f, 4.0f));
        headband.setOrigin(sf::Vector2f(headRad * 0.95f, 2.0f));
        headband.setPosition(sf::Vector2f(headPos.x, headPos.y - headRad * 0.95f));
        headband.setFillColor(character.headwearAccentColor);
        window.draw(headband);

        // Near earcup
        sf::CircleShape nearCup(8.0f);
        nearCup.setScale(sf::Vector2f(0.60f, 1.25f));
        nearCup.setOrigin(sf::Vector2f(8.0f, 8.0f));
        nearCup.setPosition(sf::Vector2f(headPos.x + dir * headRad * 0.15f, headPos.y + 1.0f));
        nearCup.setFillColor(character.headwearColor);
        nearCup.setOutlineColor(character.headwearAccentColor);
        nearCup.setOutlineThickness(2.0f);
        window.draw(nearCup);

        // Boom microphone curving toward mouth
        sf::RectangleShape micArm(sf::Vector2f(16.0f, 2.2f));
        micArm.setOrigin(sf::Vector2f(0.0f, 1.1f));
        micArm.setPosition(sf::Vector2f(headPos.x + dir * headRad * 0.15f, headPos.y + 3.0f));
        micArm.setRotation(sf::degrees(dir * 28.0f));
        micArm.setFillColor(character.headwearAccentColor);
        window.draw(micArm);

        // Mic capsule
        sf::CircleShape micCap(2.4f);
        micCap.setOrigin(sf::Vector2f(2.4f, 2.4f));
        float micTipX = headPos.x + dir * (headRad * 0.15f + 14.0f);
        float micTipY = headPos.y + 10.0f;
        micCap.setPosition(sf::Vector2f(micTipX, micTipY));
        micCap.setFillColor(sf::Color(20, 20, 24));
        window.draw(micCap);
    }
    // 3. Top Hat (Toppat Clan / RHM)
    else if (character.headwear == HeadwearType::TopHat) {
        float hatBaseY = headPos.y - headRad * 0.88f;

        // Hat Brim
        sf::RectangleShape brim(sf::Vector2f(headRad * 2.3f, 4.0f));
        brim.setOrigin(sf::Vector2f(headRad * 1.15f, 2.0f));
        brim.setPosition(sf::Vector2f(headPos.x, hatBaseY));
        brim.setFillColor(character.headwearColor);
        brim.setOutlineColor(character.headOutlineColor);
        brim.setOutlineThickness(1.5f);
        window.draw(brim);

        // Hat Crown
        sf::RectangleShape crown(sf::Vector2f(headRad * 1.4f, 20.0f));
        crown.setOrigin(sf::Vector2f(headRad * 0.7f, 20.0f));
        crown.setPosition(sf::Vector2f(headPos.x, hatBaseY));
        crown.setFillColor(character.headwearColor);
        crown.setOutlineColor(character.headOutlineColor);
        crown.setOutlineThickness(1.5f);
        window.draw(crown);

        // Ribbon band
        sf::RectangleShape band(sf::Vector2f(headRad * 1.4f, 4.5f));
        band.setOrigin(sf::Vector2f(headRad * 0.7f, 4.5f));
        band.setPosition(sf::Vector2f(headPos.x, hatBaseY));
        band.setFillColor(character.headwearAccentColor);
        window.draw(band);
    }
    // 4. Double Top Hat (Reginald Copperbottom)
    else if (character.headwear == HeadwearType::DoubleTopHat) {
        float hatBaseY = headPos.y - headRad * 0.88f;

        // Bottom Hat Brim
        sf::RectangleShape brim1(sf::Vector2f(headRad * 2.3f, 4.0f));
        brim1.setOrigin(sf::Vector2f(headRad * 1.15f, 2.0f));
        brim1.setPosition(sf::Vector2f(headPos.x, hatBaseY));
        brim1.setFillColor(character.headwearColor);
        brim1.setOutlineColor(character.headOutlineColor);
        brim1.setOutlineThickness(1.5f);
        window.draw(brim1);

        // Bottom Hat Crown
        sf::RectangleShape crown1(sf::Vector2f(headRad * 1.4f, 18.0f));
        crown1.setOrigin(sf::Vector2f(headRad * 0.7f, 18.0f));
        crown1.setPosition(sf::Vector2f(headPos.x, hatBaseY));
        crown1.setFillColor(character.headwearColor);
        crown1.setOutlineColor(character.headOutlineColor);
        crown1.setOutlineThickness(1.5f);
        window.draw(crown1);

        // Bottom Gold Band
        sf::RectangleShape band1(sf::Vector2f(headRad * 1.4f, 4.5f));
        band1.setOrigin(sf::Vector2f(headRad * 0.7f, 4.5f));
        band1.setPosition(sf::Vector2f(headPos.x, hatBaseY));
        band1.setFillColor(character.headwearAccentColor);
        window.draw(band1);

        // Stacked Top Hat Brim
        float stackBaseY = hatBaseY - 18.0f;
        sf::RectangleShape brim2(sf::Vector2f(headRad * 1.8f, 3.5f));
        brim2.setOrigin(sf::Vector2f(headRad * 0.9f, 1.75f));
        brim2.setPosition(sf::Vector2f(headPos.x, stackBaseY));
        brim2.setFillColor(character.headwearColor);
        brim2.setOutlineColor(character.headOutlineColor);
        brim2.setOutlineThickness(1.5f);
        window.draw(brim2);

        // Stacked Top Hat Crown
        sf::RectangleShape crown2(sf::Vector2f(headRad * 1.25f, 16.0f));
        crown2.setOrigin(sf::Vector2f(headRad * 0.625f, 16.0f));
        crown2.setPosition(sf::Vector2f(headPos.x, stackBaseY));
        crown2.setFillColor(character.headwearColor);
        crown2.setOutlineColor(character.headOutlineColor);
        crown2.setOutlineThickness(1.5f);
        window.draw(crown2);

        // Stacked Gold Band
        sf::RectangleShape band2(sf::Vector2f(headRad * 1.25f, 4.0f));
        band2.setOrigin(sf::Vector2f(headRad * 0.625f, 4.0f));
        band2.setPosition(sf::Vector2f(headPos.x, stackBaseY));
        band2.setFillColor(character.headwearAccentColor);
        window.draw(band2);

        // Reginald's Golden Chain across shoulder
        sf::RectangleShape chain(sf::Vector2f(16.0f, 2.5f));
        chain.setOrigin(sf::Vector2f(0.0f, 1.25f));
        chain.setPosition(sf::Vector2f(headPos.x, headPos.y + headRad * 0.95f));
        chain.setRotation(sf::degrees(dir * 25.0f));
        chain.setFillColor(character.headwearAccentColor);
        window.draw(chain);
    }
}

void RagdollRenderer::drawFace(sf::RenderWindow& window, const sf::Vector2f& headPos, float headRad, float headAngleRad, int facingDir, const CharacterDefinition& character) {
    (void)headAngleRad;
    float dir = static_cast<float>(facingDir);
    float faceCenterX = headPos.x + dir * headRad * 0.32f;
    float faceCenterY = headPos.y - 1.0f;

    // Glowing rage eyes FX if active (EWGF / electric burst state)
    if (character.glowingEyes) {
        sf::CircleShape glow(7.0f);
        glow.setOrigin(sf::Vector2f(7.0f, 7.0f));
        glow.setPosition(sf::Vector2f(faceCenterX, faceCenterY));
        glow.setFillColor(sf::Color(character.eyeGlowColor.r, character.eyeGlowColor.g, character.eyeGlowColor.b, 130));
        window.draw(glow);

        sf::CircleShape core(3.5f);
        core.setOrigin(sf::Vector2f(3.5f, 3.5f));
        core.setPosition(sf::Vector2f(faceCenterX, faceCenterY));
        core.setFillColor(character.eyeGlowColor);
        window.draw(core);
        return;
    }

    float eyeSpacing = 5.2f * dir;
    float eyeRad = 3.2f;

    // -------------------------------------------------------------
    // 1. FLASH-ACCURATE SMOOTH OVAL PILL EYES
    // -------------------------------------------------------------
    if (character.eyeStyle == EyeStyle::CyborgLaserEye) {
        // Right Hand Man Cybernetic Laser Optic
        sf::CircleShape optic(4.5f);
        optic.setOrigin(sf::Vector2f(4.5f, 4.5f));
        optic.setPosition(sf::Vector2f(faceCenterX + eyeSpacing * 0.3f, faceCenterY));
        optic.setFillColor(sf::Color(255, 30, 40));
        optic.setOutlineColor(sf::Color(18, 18, 22));
        optic.setOutlineThickness(1.8f);
        window.draw(optic);

        sf::CircleShape laserDot(2.0f);
        laserDot.setOrigin(sf::Vector2f(2.0f, 2.0f));
        laserDot.setPosition(sf::Vector2f(faceCenterX + eyeSpacing * 0.3f, faceCenterY));
        laserDot.setFillColor(sf::Color::White);
        window.draw(laserDot);

        // Far regular eye
        sf::CircleShape farEye(eyeRad * 0.85f);
        farEye.setScale(sf::Vector2f(0.68f, 1.35f));
        farEye.setOrigin(sf::Vector2f(eyeRad * 0.85f, eyeRad * 0.85f));
        farEye.setPosition(sf::Vector2f(faceCenterX - eyeSpacing * 0.7f, faceCenterY));
        farEye.setFillColor(character.headOutlineColor);
        window.draw(farEye);
    } else {
        // Smooth vertical oval cartoon eyes
        sf::CircleShape farEye(eyeRad * 0.88f);
        farEye.setScale(sf::Vector2f(0.68f, 1.35f));
        farEye.setOrigin(sf::Vector2f(eyeRad * 0.88f, eyeRad * 0.88f));
        farEye.setPosition(sf::Vector2f(faceCenterX - eyeSpacing * 0.7f, faceCenterY));
        farEye.setFillColor(character.headOutlineColor);
        window.draw(farEye);

        sf::CircleShape nearEye(eyeRad);
        nearEye.setScale(sf::Vector2f(0.68f, 1.35f));
        nearEye.setOrigin(sf::Vector2f(eyeRad, eyeRad));
        nearEye.setPosition(sf::Vector2f(faceCenterX + eyeSpacing * 0.3f, faceCenterY));
        nearEye.setFillColor(character.headOutlineColor);
        window.draw(nearEye);

        // Specular white highlight gleam dots
        sf::CircleShape gleamFar(0.9f);
        gleamFar.setOrigin(sf::Vector2f(0.9f, 0.9f));
        gleamFar.setPosition(sf::Vector2f(faceCenterX - eyeSpacing * 0.7f + dir * 0.6f, faceCenterY - 1.4f));
        gleamFar.setFillColor(sf::Color::White);
        window.draw(gleamFar);

        sf::CircleShape gleamNear(1.1f);
        gleamNear.setOrigin(sf::Vector2f(1.1f, 1.1f));
        gleamNear.setPosition(sf::Vector2f(faceCenterX + eyeSpacing * 0.3f + dir * 0.7f, faceCenterY - 1.5f));
        gleamNear.setFillColor(sf::Color::White);
        window.draw(gleamNear);

        // Ellie Rose Feminine Eyelash Flick
        if (character.eyeStyle == EyeStyle::FeminineLash) {
            sf::RectangleShape lash(sf::Vector2f(3.5f, 1.8f));
            lash.setOrigin(sf::Vector2f(0.0f, 0.9f));
            lash.setPosition(sf::Vector2f(faceCenterX + eyeSpacing * 0.3f + dir * 1.6f, faceCenterY - 3.2f));
            lash.setRotation(sf::degrees(dir * -38.0f));
            lash.setFillColor(character.headOutlineColor);
            window.draw(lash);
        }
    }

    // -------------------------------------------------------------
    // 2. EYEBROWS
    // -------------------------------------------------------------
    float browAngle = (character.eyebrowStyle == EyebrowStyle::Angry) ? 32.0f
                    : (character.eyebrowStyle == EyebrowStyle::Arched) ? 16.0f
                    : 20.0f;

    sf::RectangleShape farBrow(sf::Vector2f(6.5f, 2.0f));
    farBrow.setOrigin(sf::Vector2f(3.25f, 1.0f));
    farBrow.setPosition(sf::Vector2f(faceCenterX - eyeSpacing * 0.7f, faceCenterY - 5.5f));
    farBrow.setRotation(sf::degrees(dir * browAngle * 0.85f));
    farBrow.setFillColor(character.headOutlineColor);
    window.draw(farBrow);

    sf::RectangleShape nearBrow(sf::Vector2f(7.2f, 2.2f));
    nearBrow.setOrigin(sf::Vector2f(3.6f, 1.1f));
    nearBrow.setPosition(sf::Vector2f(faceCenterX + eyeSpacing * 0.3f, faceCenterY - 5.8f));
    nearBrow.setRotation(sf::degrees(dir * browAngle));
    nearBrow.setFillColor(character.headOutlineColor);
    window.draw(nearBrow);

    // -------------------------------------------------------------
    // 3. FACIAL HAIR
    // -------------------------------------------------------------
    if (character.facialHair == FacialHairType::ReginaldMustache) {
        // Reginald's Luxurious Golden Handlebar Mustache
        sf::CircleShape knot(3.5f);
        knot.setOrigin(sf::Vector2f(3.5f, 3.5f));
        knot.setPosition(sf::Vector2f(faceCenterX, faceCenterY + 4.5f));
        knot.setFillColor(character.facialHairColor);
        knot.setOutlineColor(character.headOutlineColor);
        knot.setOutlineThickness(1.2f);
        window.draw(knot);

        // Sweeping upward wings
        sf::RectangleShape wingNear(sf::Vector2f(9.5f, 3.2f));
        wingNear.setOrigin(sf::Vector2f(0.0f, 1.6f));
        wingNear.setPosition(sf::Vector2f(faceCenterX + dir * 1.5f, faceCenterY + 4.5f));
        wingNear.setRotation(sf::degrees(dir * -24.0f));
        wingNear.setFillColor(character.facialHairColor);
        wingNear.setOutlineColor(character.headOutlineColor);
        wingNear.setOutlineThickness(1.2f);
        window.draw(wingNear);

        sf::RectangleShape wingFar(sf::Vector2f(8.5f, 3.0f));
        wingFar.setOrigin(sf::Vector2f(8.5f, 1.5f));
        wingFar.setPosition(sf::Vector2f(faceCenterX - dir * 1.5f, faceCenterY + 4.5f));
        wingFar.setRotation(sf::degrees(dir * 22.0f));
        wingFar.setFillColor(character.facialHairColor);
        wingFar.setOutlineColor(character.headOutlineColor);
        wingFar.setOutlineThickness(1.2f);
        window.draw(wingFar);
    }
    else if (character.facialHair == FacialHairType::RHMBigMustache) {
        // Right Hand Man Bushy Ginger Mustache
        sf::RectangleShape mustache(sf::Vector2f(14.0f, 4.0f));
        mustache.setOrigin(sf::Vector2f(7.0f, 2.0f));
        mustache.setPosition(sf::Vector2f(faceCenterX, faceCenterY + 4.5f));
        mustache.setFillColor(character.facialHairColor);
        mustache.setOutlineColor(character.headOutlineColor);
        mustache.setOutlineThickness(1.4f);
        window.draw(mustache);
    }
    else {
        // -------------------------------------------------------------
        // 4. MOUTH (When not covered by mustache)
        // -------------------------------------------------------------
        if (character.mouthStyle == MouthStyle::ConfidentGrin) {
            sf::RectangleShape mouth(sf::Vector2f(6.0f, 1.8f));
            mouth.setOrigin(sf::Vector2f(3.0f, 0.9f));
            mouth.setPosition(sf::Vector2f(faceCenterX, faceCenterY + 5.8f));
            mouth.setRotation(sf::degrees(dir * 12.0f));
            mouth.setFillColor(character.headOutlineColor);
            window.draw(mouth);
        } else if (character.mouthStyle == MouthStyle::DeterminedLine) {
            sf::RectangleShape mouth(sf::Vector2f(5.5f, 1.8f));
            mouth.setOrigin(sf::Vector2f(2.75f, 0.9f));
            mouth.setPosition(sf::Vector2f(faceCenterX, faceCenterY + 5.5f));
            mouth.setFillColor(character.headOutlineColor);
            window.draw(mouth);
        } else if (character.mouthStyle == MouthStyle::Frown) {
            sf::RectangleShape mouth(sf::Vector2f(6.0f, 1.8f));
            mouth.setOrigin(sf::Vector2f(3.0f, 0.9f));
            mouth.setPosition(sf::Vector2f(faceCenterX, faceCenterY + 6.0f));
            mouth.setRotation(sf::degrees(dir * -12.0f));
            mouth.setFillColor(character.headOutlineColor);
            window.draw(mouth);
        } else {
            // Default Smirk
            sf::RectangleShape smirk(sf::Vector2f(6.0f, 1.8f));
            smirk.setOrigin(sf::Vector2f(3.0f, 0.9f));
            smirk.setPosition(sf::Vector2f(faceCenterX, faceCenterY + 6.0f));
            smirk.setRotation(sf::degrees(dir * 14.0f));
            smirk.setFillColor(character.headOutlineColor);
            window.draw(smirk);
        }
    }
}

void RagdollRenderer::drawHead(sf::RenderWindow& window, b2BodyId headBody, int facingDir, const CharacterDefinition& character) {
    if (!b2Body_IsValid(headBody)) return;

    sf::Vector2f pos = PhysicsUnits::toPixels(b2Body_GetPosition(headBody));
    b2Rot rot = b2Body_GetRotation(headBody);
    float rad = b2Rot_GetAngle(rot);
    float radius = RagdollSkeleton::HEAD_RADIUS;

    // 1. Back cosmetics (hair mane, far earcups, etc.)
    drawBackCosmetics(window, pos, radius, facingDir, character);

    // 2. Head circle (Flash crisp black outline and white fill)
    sf::CircleShape headCircle(radius);
    headCircle.setOrigin(sf::Vector2f(radius, radius));
    headCircle.setPosition(pos);
    headCircle.setRotation(sf::radians(rad));
    headCircle.setFillColor(character.headFillColor);
    headCircle.setOutlineColor(character.headOutlineColor);
    headCircle.setOutlineThickness(2.8f);
    window.draw(headCircle);

    // 3. Front cosmetics (front bangs, pilot headphones, top hats, etc.)
    drawFrontCosmetics(window, pos, radius, facingDir, character);

    // 4. Expressive Face (Flash oval pill eyes, gleams, brows, mouth/mustache)
    drawFace(window, pos, radius, rad, facingDir, character);
}

void RagdollRenderer::drawPortrait(sf::RenderWindow& window, const sf::Vector2f& pos, float radius, int facingDir, const CharacterDefinition& character) {
    // 1. Back cosmetics (hair mane, far earcups, etc.)
    drawBackCosmetics(window, pos, radius, facingDir, character);

    // 2. Head circle (Flash crisp black outline and white fill)
    sf::CircleShape headCircle(radius);
    headCircle.setOrigin(sf::Vector2f(radius, radius));
    headCircle.setPosition(pos);
    headCircle.setFillColor(character.headFillColor);
    headCircle.setOutlineColor(character.headOutlineColor);
    headCircle.setOutlineThickness(std::max(2.0f, radius * 0.12f));
    window.draw(headCircle);

    // 3. Front cosmetics (front bangs, pilot headphones, top hats, etc.)
    drawFrontCosmetics(window, pos, radius, facingDir, character);

    // 4. Expressive Face (Flash oval pill eyes, gleams, brows, mouth/mustache)
    drawFace(window, pos, radius, 0.0f, facingDir, character);
}

void RagdollRenderer::drawDropShadow(sf::RenderWindow& window, const RagdollSkeleton& skeleton, float groundY) {
    sf::Vector2f pos = skeleton.getPositionPixels();
    float heightAboveGround = std::max(0.0f, groundY - pos.y);
    float t = std::clamp(heightAboveGround / 250.0f, 0.0f, 1.0f);

    float radiusX = 26.0f * (1.0f - t * 0.45f);
    float radiusY = 7.0f * (1.0f - t * 0.45f);
    std::uint8_t alpha = static_cast<std::uint8_t>((1.0f - t) * 110.0f);

    if (alpha > 5) {
        sf::CircleShape shadow(radiusX);
        shadow.setScale(sf::Vector2f(1.0f, radiusY / radiusX));
        shadow.setOrigin(sf::Vector2f(radiusX, radiusX));
        shadow.setPosition(sf::Vector2f(pos.x, groundY));
        shadow.setFillColor(sf::Color(0, 0, 0, alpha));
        window.draw(shadow);
    }
}

void RagdollRenderer::draw(sf::RenderWindow& window, const RagdollSkeleton& skeleton, int facingDir, const CharacterDefinition& character) {
    LimbType rearThigh = (facingDir > 0) ? LimbType::LeftThigh : LimbType::RightThigh;
    LimbType rearShin = (facingDir > 0) ? LimbType::LeftShin : LimbType::RightShin;
    LimbType rearUpperArm = (facingDir > 0) ? LimbType::LeftUpperArm : LimbType::RightUpperArm;
    LimbType rearForearm = (facingDir > 0) ? LimbType::LeftForearm : LimbType::RightForearm;

    LimbType frontThigh = (facingDir > 0) ? LimbType::RightThigh : LimbType::LeftThigh;
    LimbType frontShin = (facingDir > 0) ? LimbType::RightShin : LimbType::LeftShin;
    LimbType frontUpperArm = (facingDir > 0) ? LimbType::RightUpperArm : LimbType::LeftUpperArm;
    LimbType frontForearm = (facingDir > 0) ? LimbType::RightForearm : LimbType::LeftForearm;

    // 1. Draw rear limbs (behind body)
    drawLimbSegment(window, skeleton.getBody(rearThigh), RagdollSkeleton::THIGH_WIDTH * 0.5f, RagdollSkeleton::THIGH_LEN * 0.5f, character.bodyColor);
    drawLimbSegment(window, skeleton.getBody(rearShin), RagdollSkeleton::SHIN_WIDTH * 0.5f, RagdollSkeleton::SHIN_LEN * 0.5f, character.bodyColor);
    drawShoe(window, skeleton.getBody(rearShin), facingDir, character);

    drawLimbSegment(window, skeleton.getBody(rearUpperArm), RagdollSkeleton::UPPER_ARM_WIDTH * 0.5f, RagdollSkeleton::UPPER_ARM_LEN * 0.5f, character.bodyColor);
    drawLimbSegment(window, skeleton.getBody(rearForearm), RagdollSkeleton::FOREARM_WIDTH * 0.5f, RagdollSkeleton::FOREARM_LEN * 0.5f, character.bodyColor);
    drawHand(window, skeleton.getBody(rearForearm), facingDir, character);

    // 2. Draw Torso & Hips
    drawLimbSegment(window, skeleton.getBody(LimbType::Hips), RagdollSkeleton::HIPS_WIDTH * 0.5f, RagdollSkeleton::HIPS_HEIGHT * 0.5f, character.bodyColor);
    drawLimbSegment(window, skeleton.getBody(LimbType::Torso), RagdollSkeleton::TORSO_WIDTH * 0.5f, RagdollSkeleton::TORSO_HEIGHT * 0.5f, character.bodyColor);

    // 3. Draw front limbs (in front of body)
    drawLimbSegment(window, skeleton.getBody(frontThigh), RagdollSkeleton::THIGH_WIDTH * 0.5f, RagdollSkeleton::THIGH_LEN * 0.5f, character.bodyColor);
    drawLimbSegment(window, skeleton.getBody(frontShin), RagdollSkeleton::SHIN_WIDTH * 0.5f, RagdollSkeleton::SHIN_LEN * 0.5f, character.bodyColor);
    drawShoe(window, skeleton.getBody(frontShin), facingDir, character);

    drawLimbSegment(window, skeleton.getBody(frontUpperArm), RagdollSkeleton::UPPER_ARM_WIDTH * 0.5f, RagdollSkeleton::UPPER_ARM_LEN * 0.5f, character.bodyColor);
    drawLimbSegment(window, skeleton.getBody(frontForearm), RagdollSkeleton::FOREARM_WIDTH * 0.5f, RagdollSkeleton::FOREARM_LEN * 0.5f, character.bodyColor);
    drawHand(window, skeleton.getBody(frontForearm), facingDir, character);

    // 4. Draw Head with modular cosmetics & authentic face
    drawHead(window, skeleton.getBody(LimbType::Head), facingDir, character);
}

} // namespace RagdollEngine
