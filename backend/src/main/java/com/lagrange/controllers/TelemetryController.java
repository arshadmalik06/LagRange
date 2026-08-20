package com.lagrange.controllers;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import com.lagrange.services.TelemetryService;

@RestController 
@RequestMapping("/api/v1/telemetry") 
public class TelemetryController {

    private final TelemetryService telemetryService;

    public TelemetryController(TelemetryService telemetryService) {
        this.telemetryService = telemetryService;
    }
    @PostMapping // Listens specifically for HTTP POST requests
    public ResponseEntity<String> receiveTelemetry(@RequestBody byte[] payload) {
        // Pass the raw binary data to the service
        telemetryService.processTelemetry(payload);
    
        // Return a 200 OK response to the C++ agent so it knows it succeeded
        return ResponseEntity.ok("Telemetry received successfully");
    }
}