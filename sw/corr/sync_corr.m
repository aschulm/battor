function sync_corr(infile)

%load highpass_filt

HIGH_HZ = 200;
LOW_HZ = 2;
SAMP_RATE_HZ = 5000;
BIT_LEN_S = 2;
FLASH_LEN_S = 1.280;
LED_MW = 147;
TIMESYNC_LEN = 32;
TIMESYNC_PN = [1,-1,-1,1,-1,1,1,-1,1,1,-1,1,1,-1,-1,-1,1,1,-1,-1,1,1,1,1,1,-1,1,-1,1,1,-1,1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,1,1,-1,1,1,1,-1,1,1,1,-1,-1,-1,-1,-1,-1,1,1,-1,-1];
TIMESYNC_LEN_SAMP = SAMP_RATE_HZ*BIT_LEN_S*TIMESYNC_LEN;
TIMESYNC_PN_SUB = LED_MW*[1,0,0,1,0,1,1,0,1,1,0,1,1,0,0,0,1,1,0,0,1,1,1,1,1,0,1,0,1,1,0,1,0,0,0,1,0,0,1,0,0,1,1,0,1,1,0,1,1,1,0,1,1,1,0,0,0,0,0,0,1,1,0,0];
THRESH = 5e6;
BIT = [ones(1,FLASH_LEN_S*SAMP_RATE_HZ) zeros(1,(BIT_LEN_S-FLASH_LEN_S)*SAMP_RATE_HZ)];

% filters are done offline now 

% low pass filter
%filt1 = designfilt('lowpassfir','PassbandFrequency',100, ...
%         'StopbandFrequency',105,'PassbandRipple',0.5, ...
%         'StopbandAttenuation',65,'DesignMethod','kaiserwin', ...
%         'SampleRate',SAMP_RATE_HZ);

% high pass filter
%filt1 = designfilt('highpassfir','StopbandFrequency',0.25, ...
%         'PassbandFrequency',0.35,'PassbandRipple',0.5, ...
%         'StopbandAttenuation',65,'DesignMethod','kaiserwin', ...
%         'SampleRate',SAMP_RATE_HZ);

% read in power measurements
file = fopen(infile);
% skip the comment lines
for i=1:5
    fgetl(file);
end
A = fscanf(file, '%f %f', [2 Inf]);
fclose(file);
A = A';

% power = v*i
A = bsxfun(@times, A(:,1), A(:,2));
A = A/1000.0;

A_filt = A;
%A_filt = filtfilt(filt1,A);
%A_filt = filtfilt(filt2,A_filt);

% resample PN sequence to match sample rate
A_corr = kron(TIMESYNC_PN(1:TIMESYNC_LEN),BIT); % TODO make the 50 clean
A_corr(A_corr == 0) = -1; 

%correlate power measuremnts with PN sequence
[C,C_lags]=xcorr(A_filt,A_corr);

% find the peaks in the correlation
[peaks, peakis] = findpeaks(C, 'MinPeakDistance',TIMESYNC_LEN_SAMP); % TODO make the 50 clean
I_peaks = find(peaks>THRESH);
A_hit = C_lags(peakis(I_peaks));

% generate the signal to subtract
file_bits = fopen([infile '-bits'], 'w');
A_corr_sub = kron(TIMESYNC_PN_SUB(1:TIMESYNC_LEN),BIT);
for x = A_corr
  fprintf(file_bits, '0 %d\n', x);   
end
fclose(file_bits);

OFFSET = 200;
file_hits = fopen([infile '-hits'], 'w');
A_sub = A;
for hit = A_hit
    fprintf(file_hits, '%d %d\n', i, hit);
    
    A_sub = A - cat(1, zeros(hit,1), A_corr_sub', zeros(length(A_sub)-hit-TIMESYNC_LEN_SAMP,1));
    
    plot(1:TIMESYNC_LEN_SAMP+OFFSET, [zeros(1,OFFSET/2) A_corr_sub zeros(1,OFFSET/2)], 1:TIMESYNC_LEN_SAMP+OFFSET, A(hit-(OFFSET/2):hit+TIMESYNC_LEN_SAMP-1+(OFFSET/2)));
    pause;
    plot(1:TIMESYNC_LEN_SAMP+OFFSET, [zeros(1,OFFSET/2) A_corr_sub zeros(1,OFFSET/2)], 1:TIMESYNC_LEN_SAMP+OFFSET, A_sub(hit-(OFFSET/2):hit+TIMESYNC_LEN_SAMP-1+(OFFSET/2)));
    pause;
end
fclose(file_hits);

file_sub = fopen([infile '-sub'], 'w');
for x = A_sub
    fprintf(file_sub, '%d\n', x); 
end
fclose(file_sub);

end
